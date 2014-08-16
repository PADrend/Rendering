/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "MeshBuilder.h"
#include "MeshUtils.h"
#include "../Mesh/Mesh.h"
#include "../Mesh/VertexAttributeIds.h"
#include "../Texture/Texture.h"
#include "../GLHeader.h"
#include <Geometry/Box.h>
#include <Geometry/BoxHelper.h>
#include <Geometry/Convert.h>
#include <Geometry/Matrix4x4.h>
#include <Geometry/Sphere.h>
#include <Geometry/Vec2.h>
#include <Geometry/Vec3.h>
#include <Util/Graphics/Color.h>
#include <Util/Graphics/ColorLibrary.h>
#include <Util/Graphics/PixelAccessor.h>
#include <cmath>
#include <map>

#ifndef M_PI
#define M_PI		3.14159265358979323846
#endif

#ifndef M_PI_2
#define M_PI_2		1.57079632679489661923
#endif


namespace Rendering {
namespace MeshUtils {

// ---------------------------------------------------------------------------------------------------------------
// static mesh creation helper

void MeshBuilder::addBox(MeshBuilder & builder, const Geometry::Box & box) {
	uint32_t nextIndex = builder.getNextIndex();
	for (uint_fast8_t s = 0; s < 6; ++s) {
		const Geometry::side_t side = static_cast<Geometry::side_t>(s);
		const Geometry::corner_t * corners = Geometry::Helper::getCornerIndices(side);
		const Geometry::Vec3 & normal = Geometry::Helper::getNormal(side);
		for (uint_fast8_t v = 0; v < 4; ++v) {
			const Geometry::Vec3 & corner = box.getCorner(corners[v]);
			builder.position(corner);
			builder.normal(normal);
			builder.addVertex();
		}
		builder.addQuad(nextIndex + 0, nextIndex + 1, nextIndex + 2, nextIndex + 3);
		nextIndex += 4;
	}
}

Mesh * MeshBuilder::createBox(const VertexDescription & vertexDesc, const Geometry::Box & box) {
	MeshBuilder builder(vertexDesc);
	builder.color(Util::ColorLibrary::WHITE);
	addBox(builder,box);
	return builder.buildMesh();
}

Mesh * MeshBuilder::createDome(const double radius /*= 100.0*/,
							   const int horiRes /*= 40*/,
							   const int vertRes /*= 40*/,
							   const double halfSphereFraction /*= 1.0*/,
							   const double imagePercentage /*= 1.0*/) {

	const double azimuth_step = 2.0 * M_PI / static_cast<double> (horiRes);
	const double elevation_step = halfSphereFraction * M_PI_2 / static_cast<double> (vertRes);
	const uint32_t numVertices = (horiRes + 1) * (vertRes + 1);
	const uint32_t numFaces = (2 * vertRes - 1) * horiRes;

	VertexDescription vertexDescription;
	vertexDescription.appendPosition3D();
	vertexDescription.appendTexCoord();
	auto mesh = new Mesh(vertexDescription, numVertices, numFaces * 3);

	MeshVertexData & vd = mesh->openVertexData();
	float * v = reinterpret_cast<float *> (vd.data());
	double azimuth = 0.0f;
	for (int k = 0; k <= horiRes; ++k) {
		double elevation = M_PI_2;
		for (int j = 0; j <= vertRes; ++j) {
			v[0] = radius * cos(elevation) * sin(azimuth); //x
			v[1] = radius * sin(elevation); //y
			v[2] = radius * cos(elevation) * cos(azimuth); //z
			v[3] = static_cast<float> (k) / static_cast<float> (horiRes); //u
			v[4] = 1.0f - static_cast<float> (j) / static_cast<float> (vertRes) * imagePercentage; //v
			v += 5;
			elevation -= elevation_step;
		}
		azimuth += azimuth_step;
	}
	vd.updateBoundingBox();

	MeshIndexData & id = mesh->openIndexData();
	uint32_t * indices = id.data();
	for (int k = 0; k < horiRes; ++k) {
		*indices++ = vertRes + 2 + (vertRes + 1) * k;
		*indices++ = 1 + (vertRes + 1) * k;
		*indices++ = 0 + (vertRes + 1) * k;

		for (int j = 1; j < vertRes; ++j) {
			*indices++ = vertRes + 2 + (vertRes + 1) * k + j;
			*indices++ = 1 + (vertRes + 1) * k + j;
			*indices++ = 0 + (vertRes + 1) * k + j;

			*indices++ = vertRes + 1 + (vertRes + 1) * k + j;
			*indices++ = vertRes + 2 + (vertRes + 1) * k + j;
			*indices++ = 0 + (vertRes + 1) * k + j;
		}
	}
	id.updateIndexRange();
	return mesh;
}

void MeshBuilder::addSphere(MeshBuilder & builder, uint32_t inclinationSegments, uint32_t azimuthSegments) {
	const uint32_t indexOffset = builder.getNextIndex();
	const double TWO_PI = 2.0 * M_PI;
	const double inclinationIncrement = M_PI / static_cast<double>(inclinationSegments);
	const double azimuthIncrement = TWO_PI / static_cast<double>(azimuthSegments);

	// Multiple "North Poles"
	builder.position(Geometry::Vec3f(0.0f, 1.0f, 0.0f));
	builder.normal(Geometry::Vec3f(0.0f, 1.0f, 0.0f));
	for(uint_fast32_t azimuth = 0; azimuth <= azimuthSegments; ++azimuth) {
		const double u = 1.0 - ((static_cast<double>(azimuth) + 0.5) / static_cast<double>(azimuthSegments));
		builder.texCoord0(Geometry::Vec2f(u, 1.0f));
		builder.addVertex();
	}

	// Multiple "South Poles"
	builder.position(Geometry::Vec3f(0.0f, -1.0f, 0.0f));
	builder.normal(Geometry::Vec3f(0.0f, -1.0f, 0.0f));
	for(uint_fast32_t azimuth = 0; azimuth <= azimuthSegments; ++azimuth) {
		const double u = 1.0 - (static_cast<double>(azimuth) / static_cast<double>(azimuthSegments));
		builder.texCoord0(Geometry::Vec2f(u, 0.0f));
		builder.addVertex();
	}

	for(uint_fast32_t inclination = 1; inclination < inclinationSegments; ++inclination) {
		// This loop runs until azimuth equals azimuthSegments, because we need the same vertex positions with different texture coordinates.
		for(uint_fast32_t azimuth = 0; azimuth <= azimuthSegments; ++azimuth) {
			const double inclinationAngle = inclinationIncrement * static_cast<double>(inclination);
			const double azimuthAngle = azimuthIncrement * static_cast<double>(azimuth);
			Geometry::Vec3f position = Geometry::Sphere_f::calcCartesianCoordinateUnitSphere(inclinationAngle, azimuthAngle);
			builder.position(position);
			builder.normal(position);
			builder.texCoord0(Geometry::Vec2f(
				1.0 - (static_cast<double>(azimuth) / static_cast<double>(azimuthSegments)),
				1.0 - (static_cast<double>(inclination) / static_cast<double>(inclinationSegments))));
			builder.addVertex();
		}
	}

	for(uint_fast32_t inclination = 1; inclination < inclinationSegments; ++inclination) {
		const uint32_t rowOffset = indexOffset + (inclination + 1) * (azimuthSegments + 1);
		for(uint_fast32_t azimuth = 0; azimuth < azimuthSegments; ++azimuth) {
			if(inclination == 1) {
				// Connect first row to north pole.
				const uint32_t northPoleIndex = indexOffset + azimuth;
				builder.addTriangle(northPoleIndex, rowOffset + azimuth + 1, rowOffset + azimuth);
			} else {
				builder.addQuad(
					rowOffset - (azimuthSegments + 1) + azimuth,
					rowOffset - (azimuthSegments + 1) + azimuth + 1,
					rowOffset + azimuth + 1,
					rowOffset + azimuth);
				if(inclination == inclinationSegments - 1) {
					// Connect last row to south pole.
					const uint32_t southPoleIndex = indexOffset + (azimuthSegments + 1) + azimuth;
					builder.addTriangle(southPoleIndex, rowOffset + azimuth, rowOffset + azimuth + 1);
				}
			}
		}
	}
}

Mesh * MeshBuilder::createSphere(const VertexDescription & vertexDesc,uint32_t inclinationSegments, uint32_t azimuthSegments) {
	MeshBuilder builder(vertexDesc);
	builder.color(Util::ColorLibrary::WHITE);
	addSphere(builder,inclinationSegments,azimuthSegments);
	return builder.buildMesh();
}

Mesh * MeshBuilder::createRingSector(float innerRadius, float outerRadius, uint8_t numSegments, float angle /* = 360 */){
	if (numSegments < 1 || innerRadius >= outerRadius) {
		return nullptr;
	}
	VertexDescription vertexDescription;
	vertexDescription.appendPosition3D();
	vertexDescription.appendNormalFloat();
	auto mesh = new Mesh(vertexDescription, numSegments * 2 + 2, 3 * numSegments * 2);
	MeshVertexData & vd = mesh->openVertexData();
	float * v = reinterpret_cast<float *> (vd.data());
	const float normal[3] = { -1.0f, 0.0f, 0.0f };

	// Calculate vertices on the circle.
	const float step = Geometry::Convert::degToRad(angle) / static_cast<float> (numSegments);
	for (uint_fast8_t segment = 0; segment <= numSegments; ++segment) {
		const float segmentAngle = static_cast<float> (segment) * step;
		*v++ = 0.0f;
		*v++ = innerRadius * std::sin(segmentAngle);
		*v++ = innerRadius * std::cos(segmentAngle);
		*v++ = normal[0];
		*v++ = normal[1];
		*v++ = normal[2];
		*v++ = 0.0f;
		*v++ = outerRadius * std::sin(segmentAngle);
		*v++ = outerRadius * std::cos(segmentAngle);
		*v++ = normal[0];
		*v++ = normal[1];
		*v++ = normal[2];
	}
	vd.updateBoundingBox();

	MeshIndexData & id = mesh->openIndexData();
	uint32_t * i = id.data();
	for (uint_fast8_t segment = 0; segment < numSegments; ++segment) {
		*i++ = 0 + segment * 2;
		*i++ = 1 + segment * 2;
		*i++ = 3 + segment * 2;
		*i++ = 0 + segment * 2;
		*i++ = 3 + segment * 2;
		*i++ = 2 + segment * 2;
	}
	id.updateIndexRange();
	return mesh;
}

Mesh * MeshBuilder::createDiscSector(float radius, uint8_t numSegments, float angle /* = 360 */) {
	if (numSegments < 1) {
		return nullptr;
	}
	VertexDescription vertexDescription;
	vertexDescription.appendPosition3D();
	vertexDescription.appendNormalFloat();
	MeshBuilder b(vertexDescription);
	b.normal(Geometry::Vec3(-1.0f, 0.0f, 0.0f));
	b.position(Geometry::Vec3(0,0,0));
	b.addVertex();

	// Calculate vertices on the circle.
	const float step = Geometry::Convert::degToRad(angle) / static_cast<float> (numSegments);
	for (uint_fast8_t segment = 0; segment <= numSegments; ++segment) {
		const float segmentAngle = static_cast<float> (segment) * step;
		b.position( Geometry::Vec3(0,radius * std::sin(segmentAngle),radius * std::cos(segmentAngle)) );
		b.addVertex();
	}
	for (uint_fast8_t segment = 1; segment <= numSegments; ++segment)
		b.addTriangle(0,segment,segment+1);
	return b.buildMesh();
}

Mesh * MeshBuilder::createCone(float radius, float height, uint8_t numSegments) {
	if (numSegments < 2) {
		return nullptr;
	}
	VertexDescription vertexDescription;
	vertexDescription.appendPosition3D();
	vertexDescription.appendNormalFloat();
	auto mesh = new Mesh(vertexDescription, numSegments + 1, 3 * numSegments);

	MeshVertexData & vd = mesh->openVertexData();
	float * v = reinterpret_cast<float *> (vd.data());


	// First vertex is the apex.
	const Geometry::Vec3f apex(height, 0.0f, 0.0f);
	*v++ = apex.getX();
	*v++ = apex.getY();
	*v++ = apex.getZ();
	*v++ = 1.0f;
	*v++ = 0.0f;
	*v++ = 0.0f;
	// Calculate vertices of the base.
	const float step = 6.28318530717958647692f / static_cast<float> (numSegments);
	for (uint_fast8_t segment = 0; segment < numSegments; ++segment) {
		const float angle = static_cast<float> (segment) * step;
		const Geometry::Vec3f pos(0.0f, radius * std::sin(angle), radius * std::cos(angle));
		const Geometry::Vec3f tangent(0.0f, pos.getZ(), -pos.getY());
		const Geometry::Vec3f lateral = apex - pos;
		Geometry::Vec3f normal = lateral.cross(tangent);
		normal.normalize();
		*v++ = pos.getX();
		*v++ = pos.getY();
		*v++ = pos.getZ();
		*v++ = normal.getX();
		*v++ = normal.getY();
		*v++ = normal.getZ();
	}
	vd.updateBoundingBox();

	MeshIndexData & id = mesh->openIndexData();
	uint32_t * i = id.data();
	for (uint_fast8_t segment = 1; segment < numSegments; ++segment) {
		*i++ = segment;
		*i++ = 0;
		*i++ = segment + 1;
	}
	// Connect triangles to the vertex of the first triangle.
	*i++ = numSegments;
	*i++ = 0;
	*i++ = 1;

	id.updateIndexRange();
	return mesh;
}

Mesh * MeshBuilder::createConicalFrustum(float radiusBottom, float radiusTop, float height, uint8_t numSegments) {
	if (numSegments < 2) {
		return nullptr;
	}
	VertexDescription vertexDescription;
	vertexDescription.appendPosition3D();
	vertexDescription.appendNormalFloat();
	auto mesh = new Mesh(vertexDescription, 2 * numSegments, 3 * 2 * numSegments);

	MeshVertexData & vd = mesh->openVertexData();
	float * v = reinterpret_cast<float *> (vd.data());

	const float step = 6.28318530717958647692f / static_cast<float> (numSegments);
	for (uint_fast8_t segment = 0; segment < numSegments; ++segment) {
		const float angle = static_cast<float> (segment) * step;
		const float sinAngle = std::sin(angle);
		const float cosAngle = std::cos(angle);

		const Geometry::Vec3f posBottom(0.0f, radiusBottom * sinAngle, radiusBottom * cosAngle);
		const Geometry::Vec3f posTop(height, radiusTop * sinAngle, radiusTop * cosAngle);
		const Geometry::Vec3f tangent(0.0f, posBottom.getZ(), -posBottom.getY());
		const Geometry::Vec3f lateral = posTop - posBottom;
		Geometry::Vec3f normal = lateral.cross(tangent);
		normal.normalize();
		// Set vertex on the bottom circle.
		*v++ = posBottom.getX();
		*v++ = posBottom.getY();
		*v++ = posBottom.getZ();
		*v++ = normal.getX();
		*v++ = normal.getY();
		*v++ = normal.getZ();
		// Set vertex on the top circle.
		*v++ = posTop.getX();
		*v++ = posTop.getY();
		*v++ = posTop.getZ();
		*v++ = normal.getX();
		*v++ = normal.getY();
		*v++ = normal.getZ();
	}
	vd.updateBoundingBox();

	MeshIndexData & id = mesh->openIndexData();
	uint32_t * i = id.data();
	for (int_fast16_t segment = 0; segment < 2 * (numSegments - 1); segment += 2) {
		*i++ = segment;
		*i++ = segment + 1;
		*i++ = segment + 2;

		*i++ = segment + 2;
		*i++ = segment + 1;
		*i++ = segment + 3;
	}
	// Connect last two triangles to the vertices of the first triangle.
	*i++ = 2 * numSegments - 2;
	*i++ = 2 * numSegments - 1;
	*i++ = 0;

	*i++ = 0;
	*i++ = 2 * numSegments - 1;
	*i++ = 1;

	id.updateIndexRange();
	return mesh;
}

Mesh * MeshBuilder::createArrow(float radius, float length){
	std::deque<Mesh *> meshes;
	std::deque<Geometry::Matrix4x4> transformations;
	Geometry::Matrix4x4 transform;

	meshes.push_back(createConicalFrustum(radius, radius, length - 0.01f - 0.29f, 16));
	transformations.push_back(transform);

	meshes.push_back(createConicalFrustum(radius, 2.0f * radius, 0.01f, 16));
	transform.translate(length - 0.29f - 0.01f, 0.0f, 0.0f);
	transformations.push_back(transform);

	meshes.push_back(createCone(2.0f * radius, 0.29f, 16));
	transform.translate(0.01f, 0.0f, 0.0f);
	transformations.push_back(transform);

	Mesh * arrow = MeshUtils::combineMeshes(meshes, transformations);
	MeshUtils::optimizeIndices(arrow);
	return arrow;
}

Mesh * MeshBuilder::createRectangle(const VertexDescription & desc,float width, float height){
	MeshBuilder b(desc);
	b.normal( Geometry::Vec3(0,0,1.0f) );
	// Set color for all vertices to white.
	b.color(Util::ColorLibrary::WHITE);

	b.texCoord0( Geometry::Vec2(0,0) );
	b.position( Geometry::Vec3(-width*0.5,-height*0.5,0) );
	b.addVertex();

	b.texCoord0( Geometry::Vec2(0,1) );
	b.position( Geometry::Vec3(-width*0.5,height*0.5,0) );
	b.addVertex();

	b.texCoord0( Geometry::Vec2(1,1) );
	b.position( Geometry::Vec3(width*0.5,height*0.5,0) );
	b.addVertex();

	b.texCoord0( Geometry::Vec2(1,0) );
	b.position( Geometry::Vec3(width*0.5,-height*0.5,0) );
	b.addVertex();

	b.addQuad(0,1,2,3);
	return b.buildMesh();
}


//! (static)
Mesh * MeshBuilder::createMeshFromBitmaps(const VertexDescription & d,
										  Util::Reference<Util::Bitmap> depth,
										  Util::Reference<Util::Bitmap> color,
										  Util::Reference<Util::Bitmap> normals) {
	using namespace Geometry;

	const uint32_t width = depth->getWidth();
	const uint32_t height = depth->getHeight();

	Util::Reference<Util::PixelAccessor> depthReader = Util::PixelAccessor::create(std::move(depth));
	if( depthReader.isNull() || depthReader->getPixelFormat()!=Util::PixelFormat::MONO_FLOAT ){
		WARN("createMeshFromBitmaps: unsupported depth texture format");
		return nullptr;
	}
	Util::Reference<Util::PixelAccessor> colorReader;
	if(color.isNotNull()) {
		colorReader = Util::PixelAccessor::create(std::move(color));
		if(colorReader.isNull() || (colorReader->getPixelFormat() != Util::PixelFormat::RGBA && colorReader->getPixelFormat() != Util::PixelFormat::RGB)) {
			WARN("createMeshFromBitmaps: unsupported color texture format");
			return nullptr;
		}
	}
	Util::Reference<Util::PixelAccessor> normalReader;
	if(normals.isNotNull()) {
		normalReader = Util::PixelAccessor::create(std::move(normals));
		if(normalReader.isNull()){
			WARN("createMeshFromBitmaps: unsupported normal texture format");
			return nullptr;
		}
	}

	MeshBuilder builder(d);

	const float xScale=2.0 / width;
	const float yScale=2.0 / height;
	const float cut=1;

	for(uint32_t y=0; y<height; ++y){
		for(uint32_t x=0; x<width; ++x){
			Vec3 pos(xScale * x - 1.0f, yScale * y - 1.0f, 2.0f * depthReader->readSingleValueFloat(x,y) - 1.0f);
			builder.position( pos );
			builder.color(colorReader->readColor4ub(x, y));
			if(normalReader.isNotNull()){
				Util::Color4f tmp = normalReader->readColor4f(x,y);
				Vec3 normal=Vec3(tmp.getR()-0.5f,tmp.getG()-0.5f,tmp.getB()-0.5f);
				if(!normal.isZero())
					normal.normalize();
				builder.normal(normal);
			}

			uint32_t index=builder.addVertex();
			// add triangles
			if(x>0 && y>0){
				const float z_1_1 = depthReader->readSingleValueFloat(x - 1, y - 1);
				const float z_1_0 = depthReader->readSingleValueFloat(x - 1, y);
				const float z_0_1 = depthReader->readSingleValueFloat(x, y - 1);
				const float z_0_0 = depthReader->readSingleValueFloat(x, y);

				if( std::abs( z_0_0 - z_1_1 ) > std::abs( z_1_0 - z_0_1 ) ){
					/*
					_1_1	_0_1
					  o---o  o
					  |  /  /|
					  | /  / |
					  |/  /  |
					  o  o---o
					_1_0	_0_0

					*/
					if( z_1_1<cut && z_1_0<cut && z_0_1<cut  ){
						builder.addIndex( index-width-1);
						builder.addIndex( index-width);
						builder.addIndex( index-1);
					}
//
					if( z_0_1<cut && z_1_0<cut && z_0_0<cut  ){
						builder.addIndex( index-width);
						builder.addIndex( index);
						builder.addIndex( index-1);
					}
				}else {
					/*
					_1_1	_0_1
					  o  o---o
					  |\  \  |
					  | \  \ |
					  |  \  \|
					  o---o  o
					_1_0	_0_0

					*/
					if( z_1_1<cut && z_1_0<cut && z_0_0<cut ){
						builder.addIndex( index-width-1);
						builder.addIndex( index);
						builder.addIndex( index-1);
					}

					if( z_1_1<cut && z_0_1<cut && z_0_0<cut ){
						builder.addIndex( index);
						builder.addIndex( index-width-1);
						builder.addIndex( index-width);
					}
				}
			}
		}
	}

	return builder.buildMesh();
}

// ---------------------------------------------------------------------------------------------------------------
void MeshBuilder::MBVertex::setPosition(const VertexAttribute & attr,const Geometry::Vec2 &pos){
	if(attr.getNumValues() != 2 || attr.getDataType() != GL_FLOAT ){
		WARN("Unsupported format");
		return;
	}
	float *v=floatPtr(attr);
	*(v++) = pos.x();
	*v = pos.y();
}
void MeshBuilder::MBVertex::setPosition(const VertexAttribute & attr,const Geometry::Vec3 &pos){
	if(attr.getNumValues() < 3 || attr.getDataType() != GL_FLOAT ){
		WARN("Unsupported format");
		return;
	}
	float *v=floatPtr(attr);
	*(v++) = pos.x();
	*(v++) = pos.y();
	*v = pos.z();
}
void MeshBuilder::MBVertex::setNormal(const VertexAttribute & attr, const Geometry::Vec3f & n){
	if(attr.getNumValues() < 3){
		return;
	}else if(attr.getDataType() == GL_FLOAT){
		float *v=floatPtr( attr );
		*(v++) = n.x();
		*(v++) = n.y();
		*v = n.z();
	}else if(attr.getDataType() == GL_BYTE){
		int8_t * v = int8Ptr(attr);
		*(v++) = Geometry::Convert::toSigned<int8_t>(n.x());
		*(v++) = Geometry::Convert::toSigned<int8_t>(n.y());
		*v = Geometry::Convert::toSigned<int8_t>(n.z());
	}else{
		WARN("Unsupported format");
	}
}
void MeshBuilder::MBVertex::setNormal(const VertexAttribute & attr, const Geometry::Vec3b & n){
	if(attr.getNumValues() < 3){
		return;
	}else if(attr.getDataType() == GL_FLOAT){
		float *v=floatPtr( attr );
		*(v++) = Geometry::Convert::fromSignedTo<float, int8_t>(n.x());
		*(v++) = Geometry::Convert::fromSignedTo<float, int8_t>(n.y());
		*v = Geometry::Convert::fromSignedTo<float, int8_t>(n.z());
	}else if(attr.getDataType() == GL_BYTE){
		int8_t *v=int8Ptr(attr );
		*(v++) = n.x();
		*(v++) = n.y();
		*v = n.z();
	}else{
		WARN("Unsupported format");
	}
}
void MeshBuilder::MBVertex::setColor(const VertexAttribute & attr,const Util::Color4f &color){
	if(attr.getNumValues() < 3){
		return;
	}else if(attr.getDataType() == GL_FLOAT){
		float *v=floatPtr( attr );
		*(v++) = color.getR();
		*(v++) = color.getG();
		*(v++) = color.getB();
		if(attr.getNumValues()>3)
			*v = color.getA();
	}else if(attr.getDataType() == GL_UNSIGNED_BYTE){
		uint8_t *v=uint8Ptr( attr );
		Util::Color4ub cUb=Util::Color4ub(color);
		*(v++) = cUb.getR();
		*(v++) = cUb.getG();
		*(v++) = cUb.getB();
		if(attr.getNumValues()>3)
			*v = cUb.getA();
	}else{
		WARN("Unsupported format");
	}
}
void MeshBuilder::MBVertex::setColor(const VertexAttribute & attr, const Util::Color4ub & color) {
	if(attr.getNumValues() < 3) {
		return;
	} else if(attr.getDataType() == GL_FLOAT) {
		float * v = floatPtr(attr);
		const Util::Color4f colorFloat(color);
		*(v++) = colorFloat.getR();
		*(v++) = colorFloat.getG();
		*(v++) = colorFloat.getB();
		if(attr.getNumValues() > 3) {
			*v = colorFloat.getA();
		}
	} else if(attr.getDataType() == GL_UNSIGNED_BYTE) {
		uint8_t * v = uint8Ptr(attr);
		*(v++) = color.getR();
		*(v++) = color.getG();
		*(v++) = color.getB();
		if(attr.getNumValues() > 3) {
			*v = color.getA();
		}
	} else {
		WARN("Unsupported format");
	}
}
void MeshBuilder::MBVertex::setTex0(const VertexAttribute & attr,const Geometry::Vec2 & uv){
	if(attr.getNumValues() < 2){
		return;
	}else if(attr.getDataType() == GL_FLOAT){
		float *v=floatPtr( attr );
		*(v++) = uv.x();
		*(v++) = uv.y();
	}else{
		WARN("Unsupported format");
	}
}

// -----------------------------------------------------------------------------

MeshBuilder::MeshBuilder() :
	description(),
	posAttr(description.appendPosition3D()),
	normalAttr(description.appendNormalFloat()),
	colorAttr(description.appendColorRGBAFloat()),
	tex0Attr(description.appendTexCoord()),
	currentVertex(description.getVertexSize()) {
}

MeshBuilder::MeshBuilder(VertexDescription _description) :
	description(std::move(_description)),
	posAttr(description.getAttribute(VertexAttributeIds::POSITION)),
	normalAttr(description.getAttribute(VertexAttributeIds::NORMAL)),
	colorAttr(description.getAttribute(VertexAttributeIds::COLOR)),
	tex0Attr(description.getAttribute(VertexAttributeIds::TEXCOORD0)),
	currentVertex(description.getVertexSize()) {
}

MeshBuilder::~MeshBuilder() = default;

void MeshBuilder::position(const Geometry::Vec2 & v){
	if(transMat) {
		const auto transV = transMat->transformPosition(v.getX(), v.getY(), 0);
		currentVertex.setPosition(posAttr, Geometry::Vec2(transV.getX(), transV.getY()));
	} else {
		currentVertex.setPosition(posAttr, v);
	}
}
void MeshBuilder::position(const Geometry::Vec3 & v) {
	currentVertex.setPosition(posAttr, transMat ? transMat->transformPosition(v) : v);
}
void MeshBuilder::normal(const Geometry::Vec3f & n) {
	currentVertex.setNormal(normalAttr, transMat ? ((*transMat)*Geometry::Vec4(n,0.0)).xyz() : n);
}
void MeshBuilder::normal(const Geometry::Vec3b & n) {
	if(transMat){
		normal( Geometry::Vec3f(n) );
	}else{
		currentVertex.setNormal(normalAttr, n);
	}
}

uint32_t MeshBuilder::addVertex(const Geometry::Vec3& pos, const Geometry::Vec3& n,
								float r, float g, float b, float a,
								float u, float v) {
	position(pos);
	normal(n);
	color(Util::Color4f(r,g,b,a));
	texCoord0(Geometry::Vec2(u,v));
	return addVertex();
}

uint32_t MeshBuilder::addVertex(){
	verts.emplace_back(currentVertex, description.getVertexSize());
	return verts.size() - 1;
}

void MeshBuilder::addIndex(uint32_t idx) {
	inds.push_back(idx);
}

Mesh* MeshBuilder::buildMesh() {
	if(isEmpty()) {
		std::cerr << "Empty Mesh..? (MeshBuilder::buildMesh)\n";
		return nullptr;
	}
	auto m = new Mesh(description, verts.size(), inds.size());

	// vertices
	MeshVertexData & vd=m->openVertexData();
	uint8_t * vData=vd.data();
	const size_t vSize=description.getVertexSize();
	for(const auto & vertex : verts) {
		std::copy(vertex.data.get(), vertex.data.get() + vSize, vData);
		vData += vSize;
	}
	vd.updateBoundingBox();
	verts.clear();

	if(inds.empty()){
		m->setUseIndexData(false);

	}else{
		// indices
		MeshIndexData & id=m->openIndexData();
		std::copy(inds.begin(),inds.end(),id.data());
		id.updateIndexRange();
		inds.clear();		
	}

	return m;
}

void MeshBuilder::addQuad(uint32_t idx0, uint32_t idx1,	uint32_t idx2, uint32_t idx3) {
	// 0-1
	// |/|
	// 3-2
	addIndex(idx0);
	addIndex(idx1);
	addIndex(idx3);
	addIndex(idx1);
	addIndex(idx2);
	addIndex(idx3);
}

void MeshBuilder::addTriangle(uint32_t idx0, uint32_t idx1,	uint32_t idx2) {
	addIndex(idx0);
	addIndex(idx1);
	addIndex(idx2);
}

Geometry::Matrix4x4 MeshBuilder::getTransformation() const {
	return transMat ? Geometry::Matrix4x4() : *transMat;
}
void MeshBuilder::setTransformation(const Geometry::Matrix4x4 & m){
	if(m.isIdentity()) {
		transMat.reset();
	} else {
		transMat.reset(new Geometry::Matrix4x4(m));
	}
}
void MeshBuilder::setTransformation(const Geometry::SRT & s) {
	setTransformation(Geometry::Matrix4x4(s));
}

}
}

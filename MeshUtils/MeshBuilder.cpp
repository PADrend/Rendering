/*
 This file is part of the Rendering library.
 Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
 Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
 Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
 Copyright (C) 2018 Sascha Brandt <sascha@brandt.graphics>
 
 This library is subject to the terms of the Mozilla Public License, v. 2.0.
 You should have received a copy of the MPL along with this library; see the
 file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "MeshBuilder.h"
#include "PrimitiveShapes.h"
#include "MeshUtils.h"
#include "../Mesh/VertexAccessor.h"
#include "../Mesh/Mesh.h"

#include <Geometry/Matrix4x4.h>

#include <Geometry/Vec2.h>
#include <Geometry/Vec3.h>
#include <Geometry/Sphere.h>
#include <Util/Graphics/Color.h>
#include <Util/Graphics/Bitmap.h>
#include <Util/Graphics/PixelAccessor.h>

#include <cmath>
#include <map>

namespace Rendering {
namespace MeshUtils {
	
static uint32_t nextPowerOfTwo(uint32_t n) {
	--n;
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	return n + 1;
}

//! Deprecated \see MeshUtils::createBox(...)
Mesh * MeshBuilder::createBox(const VertexDescription & vd, const Geometry::Box & box) {
	return MeshUtils::createBox(vd, box);
}

//! Deprecated \see MeshUtils::addBox(...)
void MeshBuilder::addBox(MeshBuilder & mb, const Geometry::Box & box) {
	MeshUtils::addBox(mb, box);
}

//! Deprecated \see MeshUtils::createDome(...)
Mesh * MeshBuilder::createDome(const double radius, const int horiRes, const int vertRes, const double halfSphereFraction, const double imagePercentage) { 
	VertexDescription vd;
	vd.appendPosition3D();
	vd.appendNormalByte();
	vd.appendColorRGBAByte();
	return MeshUtils::createDome(vd,radius,horiRes,vertRes,halfSphereFraction,imagePercentage);
}
					 
//! Deprecated \see MeshUtils::createSphere(...)
Mesh * MeshBuilder::createSphere(const VertexDescription & vd, uint32_t inclinationSegments, uint32_t azimuthSegments) {
	return MeshUtils::createSphere(vd,Geometry::Sphere_f(),inclinationSegments,azimuthSegments);
}

//! Deprecated \see MeshUtils::addSphere(...)
void MeshBuilder::addSphere(MeshBuilder & mb, const Geometry::Sphere_f & sphere, uint32_t inclinationSegments, uint32_t azimuthSegments) { 
	MeshUtils::addSphere(mb,sphere,inclinationSegments,azimuthSegments);
}

//! Deprecated \see MeshUtils::createDiscSector(...)
Mesh * MeshBuilder::createDiscSector(float radius, uint8_t numSegments, float angle) { 
	VertexDescription vd;
	vd.appendPosition3D();
	vd.appendNormalByte();
	vd.appendColorRGBAByte();
	return MeshUtils::createDiscSector(vd,radius,numSegments,angle);
}

//! Deprecated \see MeshUtils::createRingSector(...)
Mesh * MeshBuilder::createRingSector(float innerRadius, float outerRadius, uint8_t numSegments, float angle) { 
	VertexDescription vd;
	vd.appendPosition3D();
	vd.appendNormalByte();
	vd.appendColorRGBAByte();
	return MeshUtils::createRingSector(vd,innerRadius,outerRadius,numSegments,angle);
}

//! Deprecated \see MeshUtils::createCone(...)
Mesh * MeshBuilder::createCone(float radius, float height, uint8_t numSegments) { 
	VertexDescription vd;
	vd.appendPosition3D();
	vd.appendNormalByte();
	vd.appendColorRGBAByte();
	return MeshUtils::createCone(vd,radius,height,numSegments);
}

//! Deprecated \see MeshUtils::createConicalFrustum(...)
Mesh * MeshBuilder::createConicalFrustum(float radiusBottom, float radiusTop, float height, uint8_t numSegments) { 
	VertexDescription vd;
	vd.appendPosition3D();
	vd.appendNormalByte();
	vd.appendColorRGBAByte();
	return MeshUtils::createConicalFrustum(vd,radiusBottom,radiusTop,height,numSegments);
}

//! Deprecated \see MeshUtils::createArrow(...)
Mesh * MeshBuilder::createArrow(float radius, float length) {
	VertexDescription vd;
	vd.appendPosition3D();
	vd.appendNormalByte();
	vd.appendColorRGBAByte();
	return MeshUtils::createArrow(vd,radius,length);
}

//! Deprecated \see MeshUtils::createRectangle(...)
Mesh * MeshBuilder::createRectangle(const VertexDescription & vd,float width, float height) {
	return MeshUtils::createRectangle(vd,width,height);
}

//! Deprecated
Mesh * MeshBuilder::createMeshFromBitmaps(const VertexDescription& vd, Util::Reference<Util::Bitmap> depth, Util::Reference<Util::Bitmap> color, Util::Reference<Util::Bitmap> normals) {
	
	Util::Reference<Util::PixelAccessor> depthAcc = Util::PixelAccessor::create(std::move(depth));
	if( depth.isNull() || depth->getPixelFormat()!=Util::PixelFormat::MONO_FLOAT ){
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
	return MeshUtils::createMeshFromBitmaps(vd,depthAcc,colorReader,normalReader);
}
									
//! Deprecated \see MeshUtils::createHexGrid(...)
Mesh * MeshBuilder::createHexGrid(const VertexDescription & vd, float width, float height, uint32_t rows, uint32_t columns) { 
	return MeshUtils::createHexGrid(vd,width,height,rows,columns);
}

//! Deprecated \see MeshUtils::createVoxelMesh(...)
Mesh * MeshBuilder::createVoxelMesh(const VertexDescription & vd, const Util::PixelAccessor& colorAcc, uint32_t depth) { 
	return MeshUtils::createVoxelMesh(vd,colorAcc,depth);
}

//! Deprecated \see MeshUtils::createTorus(...)
Mesh * MeshBuilder::createTorus(const VertexDescription & vd, float innerRadius, float outerRadius, uint32_t majorSegments, uint32_t minorSegments) { 
	return MeshUtils::createTorus(vd,innerRadius,outerRadius,majorSegments,minorSegments);
}

//! Deprecated \see MeshUtils::addTorus(...)
void MeshBuilder::addTorus(MeshBuilder & mb, float innerRadius, float outerRadius, uint32_t majorSegments, uint32_t minorSegments) { 
	MeshUtils::addTorus(mb,innerRadius,outerRadius,majorSegments,minorSegments);
}

// -----------------------------------------------------------------------------

MeshBuilder::MeshBuilder() {
	description.appendPosition3D();
	description.appendNormalFloat();
	description.appendColorRGBAFloat();
	description.appendTexCoord();
	vData.allocate(1, description);
	iData.allocate(1);
	currentVertex.allocate(1, description);
	acc = new VertexAccessor(currentVertex);
	acc->setColor(0, Util::Color4f{1,1,1,1}); // Default color WHITE
}

MeshBuilder::MeshBuilder(VertexDescription _description) : description(std::move(_description)) {
	vData.allocate(1, description);
	iData.allocate(1);
	currentVertex.allocate(1, description);
	acc = new VertexAccessor(currentVertex);
	acc->setColor(0, Util::Color4f{1,1,1,1}); // Default color WHITE
}

MeshBuilder::~MeshBuilder() = default;

void MeshBuilder::position(const Geometry::Vec2 & v, const Util::StringIdentifier& attr) {
	if(transMat) {
		const auto transV = transMat->transformPosition(v.getX(), v.getY(), 0);
		acc->setPosition(0, transV, attr);
	} else {
		acc->setTexCoord(0, v, attr);
	}
}

void MeshBuilder::position(const Geometry::Vec3f & v, const Util::StringIdentifier& attr) {
	acc->setPosition(0, transMat ? transMat->transformPosition(v) : v, attr);
}

void MeshBuilder::position(const Geometry::Vec4 & v, const Util::StringIdentifier& attr) {
	acc->setVec4(0, transMat ? (*transMat) * v : v, attr);
}

void MeshBuilder::normal(const Geometry::Vec3f & n, const Util::StringIdentifier& attr) {
	acc->setNormal(0, transMat ? ((*transMat)*Geometry::Vec4(n,0.0)).xyz() : n, attr);
}
void MeshBuilder::normal(const Geometry::Vec3b & n, const Util::StringIdentifier& attr) {
	normal( Geometry::Vec3f(n) );
}

void MeshBuilder::color(const Util::Color4f & c, const Util::StringIdentifier& attr) {
	acc->setColor(0, c, attr);
}

void MeshBuilder::color(const Util::Color4ub & c, const Util::StringIdentifier& attr) {
	acc->setColor(0, c, attr);
}

void MeshBuilder::texCoord0(const Geometry::Vec2 & uv, const Util::StringIdentifier& attr) {
	acc->setTexCoord(0, uv, attr);
}

void MeshBuilder::values(const std::vector<float> & v, const Util::StringIdentifier& attr) {
	acc->setFloats(0, v, attr);
}

void MeshBuilder::values(const std::vector<uint32_t> & v, const Util::StringIdentifier& attr) {
	acc->setUInts(0, v, attr);
}

void MeshBuilder::value(float v, const Util::StringIdentifier& attr) {
	acc->setFloat(0, v, attr);
}

void MeshBuilder::value(uint32_t v, const Util::StringIdentifier& attr) {
	acc->setUInt(0, v, attr);
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

uint32_t MeshBuilder::addVertex() {
	if(vSize >= vData.getVertexCount())
		vData.allocate(vData.getVertexCount()*2, description);
		
	std::copy(currentVertex.data(), currentVertex.data() + description.getVertexSize(), vData.data() + vSize * description.getVertexSize());
	return vSize++;
}

void MeshBuilder::addIndex(uint32_t idx) {
	if(iSize >= iData.getIndexCount())
		iData.allocate(iData.getIndexCount()*2);
	iData[iSize++] = idx;
}

Mesh* MeshBuilder::buildMesh() {
	if(isEmpty()) {
		std::cerr << "Empty Mesh..? (MeshBuilder::buildMesh)\n";
		return nullptr;
	}
	
	vData.allocate(vSize, description);
	vData.updateBoundingBox();
  
  if(iSize > 0) {
  	iData.allocate(iSize);
  	iData.updateIndexRange();
  }

	auto m = new Mesh(iData, vData);
	if(iSize == 0)
		m->setUseIndexData(false);

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

void MeshBuilder::addMesh(Mesh* mesh) {
	if(iSize + mesh->getIndexCount() > iData.getIndexCount())
		iData.allocate(nextPowerOfTwo(iSize + mesh->getIndexCount()));
	if(vSize + mesh->getVertexCount() > vData.getVertexCount())
		vData.allocate(nextPowerOfTwo(vSize + mesh->getVertexCount()), description);
	
	const auto& id = mesh->openIndexData();
	const auto& vd = mesh->openVertexData();
	
	if(mesh->getIndexCount() > 0) {
		std::copy(id.data(), id.data() + id.getIndexCount(), iData.data() + iSize);	
		if(vSize > 0) {
			for(uint32_t i=0; i<mesh->getIndexCount(); ++i) {
				iData[iSize+i] += vSize;
			}
		}
	}
		
	if(description == mesh->getVertexDescription()) {
		std::copy(vd.data(), vd.data() + vd.dataSize(), vData.data() + vSize*description.getVertexSize());
	} else {
		std::unique_ptr<MeshVertexData> newVd(MeshUtils::convertVertices(vd, description));
		std::copy(newVd->data(), newVd->data() + newVd->dataSize(), vData.data() + vSize*description.getVertexSize());
	}
	
	if(transMat) {
		VertexAccessor va(vData);
		for(uint32_t i=0; i<mesh->getVertexCount(); ++i) {
			va.setPosition(i+vSize, transMat->transformPosition(va.getPosition(i+vSize)));
			va.setNormal(i+vSize, transMat->transformDirection(va.getNormal(i+vSize)));
		}
	}
	
	iSize += mesh->getIndexCount();
	vSize += mesh->getVertexCount();
}

Geometry::Matrix4x4 MeshBuilder::getTransformation() const {
	return !transMat ? Geometry::Matrix4x4() : *transMat;
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

void MeshBuilder::transform(const Geometry::Matrix4x4 & m) {
	setTransformation(getTransformation() * m);
}

}
}

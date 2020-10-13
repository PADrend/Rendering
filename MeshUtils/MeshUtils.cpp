/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "MeshUtils.h"
#include "../Mesh/Mesh.h"
#include "../Mesh/VertexDescription.h"
#include "../Mesh/VertexAttributeAccessors.h"
#include "../Mesh/VertexAttributeIds.h"
#include "../Helper.h"
#include "../Texture/Texture.h"
#include "../Texture/TextureUtils.h"
#include "TriangleAccessor.h"
#include <Geometry/BoundingSphere.h>
#include <Geometry/Box.h>
#include <Geometry/Matrix4x4.h>
#include <Geometry/Sphere.h>
#include <Geometry/Tools.h>
#include <Geometry/Triangle.h>
#include <Geometry/Vec2.h>
#include <Geometry/Vec3.h>
#include <Geometry/Plane.h>
#include <Geometry/Line.h>
#include <Geometry/LineTriangleIntersection.h>
#include <Geometry/PointOctree.h>
#include <Geometry/Point.h>
#include <Geometry/Interpolation.h>
#include <Util/Graphics/Color.h>
#include <Util/Graphics/PixelAccessor.h>
#include <Util/Graphics/NoiseGenerator.h>
#include <Util/Macros.h>
#include <Util/Utils.h>
#include <Util/Numeric.h>
#include <algorithm>
#include <cmath>
#include <cstring> /* for memcmp */
#include <map>
#include <memory>
#include <queue>
#include <deque>
#include <set>
#include <stack>
#include <stdexcept>
#include <vector>
#include <unordered_map>

using Geometry::Matrix4x4f;
using Geometry::Vec3f;

namespace Rendering {
namespace MeshUtils {

/**
 * Class which stores raw vertex data. Used in
 * @a eliminateDuplicateVertices().
 * @a splitLargeTriangles().
 *
 * @author Benjamin Eikel
 * @author Ralf Petring
 * @date 2009-08-10
 */
class RawVertex {
public:
	RawVertex(const uint32_t vertexIndex, const uint8_t * vertexData, const size_t dataSize) :
		index(vertexIndex), data(vertexData), size(dataSize) {
	}

	//! Return the index in the mesh of the vertex.
	uint32_t getIndex() const {
		return index;
	}

	//! Return the byte position of the vertex data.
	const uint8_t * getData() const {
		return data;
	}

	//! Return the number of bytes of the vertex data.
	size_t getSize() const {
		return size;
	}

	/**
	 * Compare this vertex byte-wise with another vertex.
	 *
	 * @param other Second vector.
	 * @return @c true if this vertex is smaller than the other,
	 * @c false otherwise.
	 */
	bool operator<(const RawVertex & other) const {
		size_t min = std::min(size, other.size);
		int result = memcmp(data, other.data, min);
		if (result < 0) {
			return true;
		} else if (result > 0) {
			return false;
		}
		return size < other.size;
	}

	/**
	 * @param rwa first RawVertex for interpolation
	 * @param rwb second RawVertex for interpolation
	 * @param newIndex the index of the created RawVertex
	 * @param vd the VertexDescription of both RawVertices
	 * @return a linear interpolated RawVertex in the middle of rwa and rwb
	 * @author Ralf Petring
	 */
	static RawVertex midPoint(const RawVertex & rwa, const RawVertex & rwb, const uint32_t & newIndex, const VertexDescription & vd);

	/**
	 * @param rwa first RawVertex for interpolation
	 * @param rwb second RawVertex for interpolation
	 * @param a interpolation factor (between 0.0 and 1.0)
	 * @param newIndex the index of the created RawVertex
	 * @param vd the VertexDescription of both RawVertices
	 * @return a linear interpolated RawVertex in the middle of rwa and rwb
	 * @author Sascha Brandt
	 */
	static RawVertex interpolate(const RawVertex & rwa, const RawVertex & rwb, float a, const uint32_t & newIndex, const VertexDescription & vd);

	static RawVertex move(const RawVertex & rw, const Geometry::Vec3 & dir, const uint32_t & newIndex, const VertexDescription & vd);

private:
	//! Index of the vertex in the mesh.
	uint32_t index;

	//! Byte position of beginning of vertex data.
	const uint8_t * data;

	//! Number of bytes of vertex data.
	size_t size;
};

/**
 * class for storing triangle data and sorting by longest side of the triangle
 * used in splitLargeTriangles()
 * @author Ralf Petring
 */
class SplitTriangle {
public:
	/// Positions of vertices have to be floats, sorry
	SplitTriangle(RawVertex _a, RawVertex _b, RawVertex _c) :
		a(std::move(_a)), b(std::move(_b)), c(std::move(_c)) {
		Geometry::Vec3 va(reinterpret_cast<const float *> (a.getData()));
		Geometry::Vec3 vb(reinterpret_cast<const float *> (b.getData()));
		Geometry::Vec3 vc(reinterpret_cast<const float *> (c.getData()));
		this->longestSideLength = 0;
		this->longestSideIndex = 0;
		float tmp = (va - vb).length();
		if (this->longestSideLength < tmp) {
			this->longestSideLength = tmp;
			this->longestSideIndex = 0;
		}
		tmp = (vb - vc).length();
		if (this->longestSideLength < tmp) {
			this->longestSideLength = tmp;
			this->longestSideIndex = 1;
		}
		tmp = (vc - va).length();
		if (this->longestSideLength < tmp) {
			this->longestSideLength = tmp;
			this->longestSideIndex = 2;
		}
	}

	RawVertex & getRawVertex(uint32_t index) {
		switch (index % 3) {
			case 0:
				return a;
			case 1:
				return b;
			case 2:
			default:
				return c;
		}
	}

	bool operator<(const SplitTriangle & other) const {
		if (this->longestSideLength != other.longestSideLength)
			return this->longestSideLength < other.longestSideLength;
		if (this->a.getIndex() != other.a.getIndex())
			return this->a.getIndex() < other.a.getIndex();
		if (this->b.getIndex() != other.b.getIndex())
			return this->b.getIndex() < other.b.getIndex();
		return this->a.getIndex() < other.a.getIndex();
	}

	RawVertex a;
	RawVertex b;
	RawVertex c;
	uint32_t longestSideIndex;
	float longestSideLength;
};

// -----------------------------------------------------------------------------

RawVertex RawVertex::midPoint(const RawVertex & rwa, const RawVertex & rwb, const uint32_t & newIndex, const VertexDescription & vd) {
	FAIL_IF(rwa.getSize()!=rwb.getSize());
	auto data = new uint8_t[rwa.getSize()];
	RawVertex ret(newIndex, data, rwa.getSize());
	for(const auto & attr : vd.getAttributes()) {
		if (attr.empty())
			continue;
		for (unsigned j = 0; j < attr.getNumValues(); ++j) {
			switch (attr.getDataType()) {
			case Util::TypeConstant::FLOAT: {
				float f = (reinterpret_cast<const float *> (rwa.getData() + attr.getOffset()))[j];
				f += (reinterpret_cast<const float *> (rwb.getData() + attr.getOffset()))[j];
				(reinterpret_cast<float *> (data + attr.getOffset()))[j] = f / 2;
			}
				break;
			case Util::TypeConstant::UINT8: {
				uint32_t ub = (reinterpret_cast<const uint8_t *> (rwa.getData() + attr.getOffset() + j * sizeof(uint8_t)))[0];
				ub += (reinterpret_cast<const uint8_t *> (rwb.getData() + attr.getOffset() + j * sizeof(uint8_t)))[0];
				(reinterpret_cast<uint8_t *> (data + attr.getOffset() + j * sizeof(uint8_t)))[0] = ub / 2;
			}
				break;
			case Util::TypeConstant::INT8: {
				int32_t sb = (reinterpret_cast<const int8_t *> (rwa.getData() + attr.getOffset() + j * sizeof(int8_t)))[0];
				sb += (reinterpret_cast<const int8_t *> (rwb.getData() + attr.getOffset() + j * sizeof(int8_t)))[0];
				(reinterpret_cast<int8_t *> (data + attr.getOffset() + j * sizeof(int8_t)))[0] = sb / 2;
			}
				break;
			case Util::TypeConstant::UINT16: {
				uint32_t us = (reinterpret_cast<const uint16_t *> (rwa.getData() + attr.getOffset() + j * sizeof(uint16_t)))[0];
				us += (reinterpret_cast<const uint16_t *> (rwb.getData() + attr.getOffset() + j * sizeof(uint16_t)))[0];
				(reinterpret_cast<uint16_t *> (data + attr.getOffset() + j * sizeof(uint16_t)))[0] = us / 2;
			}
				break;
			case Util::TypeConstant::INT16: {
				int32_t ss = (reinterpret_cast<const int16_t *> (rwa.getData() + attr.getOffset() + j * sizeof(int16_t)))[0];
				ss += (reinterpret_cast<const int16_t *> (rwb.getData() + attr.getOffset() + j * sizeof(int16_t)))[0];
				(reinterpret_cast<int16_t *> (data + attr.getOffset() + j * sizeof(int16_t)))[0] = ss / 2;
			}
				break;
			case Util::TypeConstant::UINT32: {
				uint64_t ui = (reinterpret_cast<const uint32_t *> (rwa.getData() + attr.getOffset() + j * sizeof(uint32_t)))[0];
				ui += (reinterpret_cast<const uint32_t *> (rwb.getData() + attr.getOffset() + j * sizeof(uint32_t)))[0];
				(reinterpret_cast<uint32_t *> (data + attr.getOffset() + j * sizeof(uint32_t)))[0] = ui / 2;
			}
				break;
			case Util::TypeConstant::INT32: {
				int64_t si = (reinterpret_cast<const int32_t *> (rwa.getData() + attr.getOffset() + j * sizeof(int32_t)))[0];
				si += (reinterpret_cast<const int32_t *> (rwb.getData() + attr.getOffset() + j * sizeof(int32_t)))[0];
				(reinterpret_cast<int32_t *> (data + attr.getOffset() + j * sizeof(int32_t)))[0] = si / 2;
			}
				break;
			case Util::TypeConstant::DOUBLE: {
				double d = (reinterpret_cast<const double *> (rwa.getData() + attr.getOffset() + j * sizeof(double)))[0];
				d += (reinterpret_cast<const double *> (rwb.getData() + attr.getOffset() + j * sizeof(double)))[0];
				(reinterpret_cast<double *> (data + attr.getOffset() + j * sizeof(double)))[0] = d / 2;
			}
				break;
			default:
				continue;
			}
		}
	}
	return ret;
}

// -----------------------------------------------------------------------------

template<typename Type>
inline
void interpolateValue(uint8_t* data, const RawVertex & rwa, const RawVertex & rwb, const VertexAttribute& attr, unsigned j, float a, float a_inv) {
	float f = static_cast<float>((reinterpret_cast<const Type *> (rwa.getData() + attr.getOffset() + j * sizeof(Type)))[0]) * a_inv;
	f += static_cast<float>((reinterpret_cast<const Type *> (rwb.getData() + attr.getOffset() + j * sizeof(Type)))[0]) * a;
	(reinterpret_cast<Type *> (data + attr.getOffset() + j * sizeof(Type)))[0] = static_cast<Type>(f);
}

// -----------------------------------------------------------------------------


RawVertex RawVertex::interpolate(const RawVertex & rwa, const RawVertex & rwb, float a, const uint32_t & newIndex, const VertexDescription & vd) {
	FAIL_IF(rwa.getSize()!=rwb.getSize());
	auto data = new uint8_t[rwa.getSize()];
	RawVertex ret(newIndex, data, rwa.getSize());
	float a_inv = 1.0f - a;
	for(const auto & attr : vd.getAttributes()) {
		if (attr.empty())
			continue;
		for (unsigned j = 0; j < attr.getNumValues(); ++j) {
			switch (attr.getDataType()) {
			case Util::TypeConstant::FLOAT:
				interpolateValue<float>(data, rwa, rwb, attr, j, a, a_inv);
				break;
			case Util::TypeConstant::UINT8:
				interpolateValue<uint8_t>(data, rwa, rwb, attr, j, a, a_inv);
				break;
			case Util::TypeConstant::INT8:
				break;
			case Util::TypeConstant::UINT16:
				interpolateValue<uint16_t>(data, rwa, rwb, attr, j, a, a_inv);
				break;
			case Util::TypeConstant::INT16:
				interpolateValue<int16_t>(data, rwa, rwb, attr, j, a, a_inv);
				break;
			case Util::TypeConstant::UINT32:
				interpolateValue<uint32_t>(data, rwa, rwb, attr, j, a, a_inv);
				break;
			case Util::TypeConstant::INT32:
				interpolateValue<int32_t>(data, rwa, rwb, attr, j, a, a_inv);
				break;
			case Util::TypeConstant::DOUBLE:
				interpolateValue<double>(data, rwa, rwb, attr, j, a, a_inv);
				break;
			default:
				continue;
			}
		}
	}
	return ret;
}

// -----------------------------------------------------------------------------

RawVertex RawVertex::move(const RawVertex & rw, const Geometry::Vec3 & dir, const uint32_t & newIndex, const VertexDescription & vd) {
	auto data = new uint8_t[rw.getSize()];
	std::copy(rw.getData(), rw.getData() + rw.getSize(), data);
	const VertexAttribute & attr = vd.getAttribute(VertexAttributeIds::POSITION);
	// assume float
	float* posData = reinterpret_cast<float *> (data + attr.getOffset());
	posData[0] += dir.x();
	posData[1] += dir.y();
	posData[2] += dir.z();

	RawVertex ret(newIndex, data, rw.getSize());
	return ret;
}

// -----------------------------------------------------------------------------

Geometry::Sphere_f calculateBoundingSphere(Mesh * mesh) {
	MeshVertexData & vertexData = mesh->openVertexData();
	Util::Reference<PositionAttributeAccessor> positionAccessor(PositionAttributeAccessor::create(vertexData, VertexAttributeIds::POSITION));

	const uint32_t vertexCount = vertexData.getVertexCount();
	std::vector<Geometry::Vec3f> positions;
	positions.reserve(vertexCount);

	for(uint_fast32_t v = 0; v < vertexCount; ++v) {
		positions.push_back(positionAccessor->getPosition(v));
	}

	const auto sphere = Geometry::BoundingSphere::computeMiniball(positions);
	if(!(sphere.getRadius() > 0)) {
		throw std::runtime_error("Bounding sphere with invalid radius computed.");
	}
	return sphere;
}

// -----------------------------------------------------------------------------

Geometry::Sphere_f calculateBoundingSphere(const std::vector<std::pair<Mesh *, Geometry::Matrix4x4>> & meshesAndTransformations) {
	uint32_t sumVertexCount = 0;
	for(const auto & meshTransformationPair : meshesAndTransformations) {
		sumVertexCount += meshTransformationPair.first->openVertexData().getVertexCount();
	}
	std::vector<Geometry::Vec3f> positions;
	positions.reserve(sumVertexCount);
	for(const auto & meshTransformationPair : meshesAndTransformations) {
		MeshVertexData & vertexData = meshTransformationPair.first->openVertexData();
		Util::Reference<PositionAttributeAccessor> positionAccessor(PositionAttributeAccessor::create(vertexData, VertexAttributeIds::POSITION));

		const auto & transformation = meshTransformationPair.second;
		const uint32_t vertexCount = vertexData.getVertexCount();
		for(uint_fast32_t v = 0; v < vertexCount; ++v) {
			positions.emplace_back(transformation.transformPosition(positionAccessor->getPosition(v)));
		}
	}

	const auto sphere = Geometry::BoundingSphere::computeEPOS98(positions);
	if(!(sphere.getRadius() > 0)) {
		throw std::runtime_error("Bounding sphere with invalid radius computed.");
	}
	return sphere;
}

// -----------------------------------------------------------------------------

//! (static)
uint32_t calculateHash( Mesh * mesh ){
	if(mesh==nullptr)
		return 0;

	MeshIndexData & iData = mesh->openIndexData();
	uint32_t h = Util::calcHash( reinterpret_cast<const uint8_t*>(iData.data()),iData.dataSize() );

	MeshVertexData & vData = mesh->openVertexData();
	h ^= Util::calcHash( vData.data(),vData.dataSize() );

	h ^= calculateHash( vData.getVertexDescription() );

	return h;
}

// -----------------------------------------------------------------------------

//! (static)
uint32_t calculateHash( const VertexDescription & vd ){
	uint32_t h = 0;
	for(const auto & attr : vd.getAttributes()) {
		h ^= Util::calcHash(reinterpret_cast<const uint8_t*>(&attr), sizeof(VertexAttribute));
	}
	return h;
}

// -----------------------------------------------------------------------------

//! (static)
bool compareMeshes( Mesh * mesh1,Mesh * mesh2 ){

	// identity
	if(mesh1==mesh2)
		return true;

	// properties
	if(mesh1->getIndexCount() != mesh2->getIndexCount() ||
			mesh1->getVertexCount() != mesh2->getVertexCount() ||
			!(mesh1->getVertexDescription() == mesh2->getVertexDescription()) )
		return false;

	// indices
	const MeshIndexData & iData1 = mesh1->openIndexData();
	if(!std::equal(iData1.data(),iData1.data()+iData1.getIndexCount(),mesh2->openIndexData().data()) )
		return false;

	// vertices
	const MeshVertexData & vData1 = mesh1->openVertexData();
	if(!std::equal(vData1.data(),vData1.data()+vData1.dataSize(),mesh2->openVertexData().data()) )
		return false;

	return true;
}

// -----------------------------------------------------------------------------

float getLongestSideLength(Mesh * m){
	float maxSideLength = 0.0;
	const VertexDescription & vd = m->getVertexDescription();
	const VertexAttribute & posAttr = vd.getAttribute(VertexAttributeIds::POSITION);
	std::vector<RawVertex> vertexArray;
	if (posAttr.getDataType() != Util::TypeConstant::FLOAT || m->getDrawMode() != Mesh::DRAW_TRIANGLES) {
		WARN("splitLargeTriangles: Unsupported vertex format.");
		return -1;
	}
	MeshVertexData & vertices = m->openVertexData();
	MeshIndexData & indices = m->openIndexData();

	// extract triangles
	size_t vertexSize = vd.getVertexSize();
	for (uint32_t i = 0; i < vertices.getVertexCount(); ++i) {
		auto tmpData = new uint8_t[vertexSize];
		std::copy(vertices[i], vertices[i] + vertexSize, tmpData);
		vertexArray.emplace_back(i, tmpData, vertexSize);
	}
	uint32_t * iData = indices.data();
	for (unsigned i = 0; i < indices.getIndexCount(); i += 3){
		float tmp = (SplitTriangle(vertexArray.at(iData[i + 0]), vertexArray.at(iData[i + 1]), vertexArray.at(iData[i + 2]))).longestSideLength;
		if(tmp > maxSideLength)
			maxSideLength = tmp;

	}
	return maxSideLength;
}

// -----------------------------------------------------------------------------

//! (static)
void setMaterial(Mesh * mesh, const Util::Color4f & ambient, const Util::Color4f & diffuse, const Util::Color4f & /*specular*/, float /*shininess*/) {
	setColor(mesh, Util::Color4f(ambient, diffuse, 0.2f));
}

// -----------------------------------------------------------------------------

//! (static)
void setColor(Mesh * mesh, const Util::Color4f & _color) {
	VertexDescription vDesc = mesh->getVertexDescription();
	MeshVertexData & vData = mesh->openVertexData();

	VertexAttribute colorAttr(vDesc.getAttribute(VertexAttributeIds::COLOR));
	if (colorAttr.empty()) { // no color available -> add color
		colorAttr = vDesc.appendColorRGBAByte();
		std::unique_ptr<MeshVertexData> vDataNew(convertVertices(vData, vDesc));
		vData.swap(*vDataNew);
	}
	Util::Reference<ColorAttributeAccessor> colorAccessor(ColorAttributeAccessor::create(vData,VertexAttributeIds::COLOR));
	for(uint32_t i=0;colorAccessor->checkRange(i);++i)
		colorAccessor->setColor(i,_color);
	vData.markAsChanged();

}

// -----------------------------------------------------------------------------

//! (static)
void splitLargeTriangles(Mesh * m, float maxSideLength) {
	const VertexDescription & vd = m->getVertexDescription();
	const VertexAttribute & posAttr = vd.getAttribute(VertexAttributeIds::POSITION);
	if (posAttr.getDataType() != Util::TypeConstant::FLOAT || m->getDrawMode() != Mesh::DRAW_TRIANGLES) {
		WARN("splitLargeTriangles: Unsupported vertex format.");
		return;
	}

	std::priority_queue<SplitTriangle> triangles; // todo: ?? use external comparator.
	std::vector<RawVertex> vertexArray;

	MeshVertexData & vertices = m->openVertexData();
	MeshIndexData & indices = m->openIndexData();

	// extract triangles
	size_t vertexSize = vd.getVertexSize();
	for (uint32_t i = 0; i < vertices.getVertexCount(); ++i) {
		auto tmpData = new uint8_t[vertexSize];
		std::copy(vertices[i], vertices[i] + vertexSize, tmpData);
		vertexArray.emplace_back(i, tmpData, vertexSize);
	}
	uint32_t * iData = indices.data();
	for (unsigned i = 0; i < indices.getIndexCount(); i += 3)
		triangles.push(SplitTriangle(vertexArray.at(iData[i + 0]), vertexArray.at(iData[i + 1]), vertexArray.at(iData[i + 2])));

	// split large triangles
	while (triangles.top().longestSideLength > maxSideLength) {
		SplitTriangle t = triangles.top();
		triangles.pop();
		RawVertex a = t.getRawVertex(t.longestSideIndex + 0);
		RawVertex b = t.getRawVertex(t.longestSideIndex + 1);
		RawVertex c = t.getRawVertex(t.longestSideIndex + 2);
		RawVertex d = RawVertex::midPoint(a, b, vertexArray.size(), vd);
		vertexArray.push_back(d);
		triangles.push(SplitTriangle(a, d, c));
		triangles.push(SplitTriangle(d, b, c));
	}

	// resemble mesh
	// - indices
	uint32_t iCount = triangles.size() * 3;
	indices.allocate(iCount);
	for (uint32_t i = 0; i < iCount; i += 3) {
		SplitTriangle t = triangles.top();
		triangles.pop();
		indices[i + 0] = t.a.getIndex();
		indices[i + 1] = t.b.getIndex();
		indices[i + 2] = t.c.getIndex();
	}
	indices.updateIndexRange();

	// - vertices
	vertices.allocate(vertexArray.size(), vd);
	for (size_t i = 0; i < vertexArray.size(); i++) {
		std::copy(vertexArray.at(i).getData(), vertexArray.at(i).getData() + vertexSize, vertices[i]);
	}
	vertices.updateBoundingBox();

	// cleanup
	for (auto & rawVertex : vertexArray) {
		delete[] rawVertex.getData();
	}
}

// -----------------------------------------------------------------------------

//! (static)
void shrinkMesh(Mesh * m, bool shrinkPosition) {
	// prepare vertex description
	const VertexDescription & vdOld = m->getVertexDescription();
	VertexDescription vdNew;

	bool convertNormals = false;
	bool convertColors = false;
	bool convertPosition = false;
	for(const auto & attr : vdOld.getAttributes()) {
		// can shrink normals?
		if (attr.getNameId() == VertexAttributeIds::NORMAL && attr.getDataType() == Util::TypeConstant::FLOAT && attr.getNumValues() >= 3) {
			vdNew.appendNormalByte();
			convertNormals = true;
		} // can shrink colors?
		else if (attr.getNameId() == VertexAttributeIds::COLOR && attr.getDataType() == Util::TypeConstant::FLOAT && attr.getNumValues() >= 3) {
			vdNew.appendColorRGBAByte();
			convertColors = true;
		} // can shrink position?
		else if (shrinkPosition && attr.getNameId() == VertexAttributeIds::POSITION && attr.getDataType() == Util::TypeConstant::FLOAT && attr.getNumValues() >= 3) {
			vdNew.appendPosition4DHalf();
			convertPosition = true;
		} else { // just copy
			vdNew.appendAttribute(attr.getNameId(), attr.getDataType(), attr.getNumValues(), attr.isNormalized());
		}
	}

	if (!convertColors && !convertNormals && !convertPosition)
		return;

	// convert vertices
	MeshVertexData & oldVertices = m->openVertexData();
	std::unique_ptr<MeshVertexData> newVertices(convertVertices(oldVertices, vdNew));

	if (convertColors) {
		Util::Reference<ColorAttributeAccessor> source(ColorAttributeAccessor::create(oldVertices,VertexAttributeIds::COLOR));
		Util::Reference<ColorAttributeAccessor> target(ColorAttributeAccessor::create(*newVertices.get(),VertexAttributeIds::COLOR));
		for(uint32_t i=0;source->checkRange(i);++i)
			target->setColor(i,source->getColor4ub(i));
	}
	if (convertNormals) {
		Util::Reference<NormalAttributeAccessor> source(NormalAttributeAccessor::create(oldVertices,VertexAttributeIds::NORMAL));
		Util::Reference<NormalAttributeAccessor> target(NormalAttributeAccessor::create(*newVertices.get(),VertexAttributeIds::NORMAL));
		for(uint32_t i=0;source->checkRange(i);++i)
			target->setNormal(i,source->getNormal(i));
	}
	if (convertPosition) {
		Util::Reference<PositionAttributeAccessor> source(PositionAttributeAccessor::create(oldVertices,VertexAttributeIds::POSITION));
		Util::Reference<PositionAttributeAccessor> target(PositionAttributeAccessor::create(*newVertices.get(),VertexAttributeIds::POSITION));
		for(uint32_t i=0;source->checkRange(i);++i)
			target->setPosition(i,source->getPosition(i));
	}
	// set new vertices
	oldVertices.swap(*newVertices.get());

	oldVertices.markAsChanged();
	oldVertices.updateBoundingBox();
}

// -----------------------------------------------------------------------------

//! (internal) Transforms a range of vertices with the given matrix.
static void transformVertexData(MeshVertexData & vData, const Matrix4x4f & transMat, uint32_t begin, uint32_t numVerts) {
	transformCoordinates(vData, VertexAttributeIds::POSITION, transMat, begin, numVerts);
	if(vData.getVertexDescription().hasAttribute(VertexAttributeIds::NORMAL))
		transformNormals(vData, VertexAttributeIds::NORMAL, transMat, begin, numVerts);
}

// -----------------------------------------------------------------------------

//! (static)
void transform(MeshVertexData & vData, const Matrix4x4f & transMat) {
	transformVertexData(vData, transMat, 0, vData.getVertexCount());
	vData.updateBoundingBox();
}

// -----------------------------------------------------------------------------

//! (static)
void transformCoordinates(MeshVertexData & vData, Util::StringIdentifier attrName, const Geometry::Matrix4x4 & transMat, uint32_t begin,
		uint32_t numVerts) {

	Util::Reference<PositionAttributeAccessor> positionAccessor(PositionAttributeAccessor::create(vData,attrName));
	const uint32_t end = begin+numVerts;
	for(uint32_t i=begin;i<end;++i)
		positionAccessor->setPosition(i,transMat.transformPosition(positionAccessor->getPosition(i)));
	vData.markAsChanged();
}

// -----------------------------------------------------------------------------

//! (static)
void transformNormals(MeshVertexData & vData, Util::StringIdentifier attrName, const Geometry::Matrix4x4 & transMat, uint32_t begin,
		uint32_t numVerts) {

	Util::Reference<NormalAttributeAccessor> normalAccessor(NormalAttributeAccessor::create(vData,attrName));
	const uint32_t end = begin+numVerts;
	for(uint32_t i=begin;i<end;++i)
		normalAccessor->setNormal(i, (transMat * Geometry::Vec4(normalAccessor->getNormal(i),0)).xyz());
	vData.markAsChanged();
}

// -----------------------------------------------------------------------------

inline bool canConvert(const VertexAttribute& oldAttr, const VertexAttribute& newAttr) {
	if(oldAttr.getDataType() == Util::TypeConstant::FLOAT) {
		return newAttr.getDataType() == Util::TypeConstant::INT8 || newAttr.getDataType() == Util::TypeConstant::UINT8; // float to byte
	} else if(newAttr.getDataType() == Util::TypeConstant::FLOAT) {
		return oldAttr.getDataType() == Util::TypeConstant::INT8 || oldAttr.getDataType() == Util::TypeConstant::UINT8; // byte to float
	}
	return false;
}

// -----------------------------------------------------------------------------

//! (static)
MeshVertexData * convertVertices(const MeshVertexData & oldVertices, const VertexDescription & newVertexDescription) {

	const VertexDescription & oldVertexDescription = oldVertices.getVertexDescription();
	if (oldVertexDescription == newVertexDescription)
		return new MeshVertexData(oldVertices);

	const uint32_t numVertices = oldVertices.getVertexCount();
	auto newVertices = new MeshVertexData;
	newVertices->allocate(numVertices, newVertexDescription);

	// Initialize the data with zero.
	std::fill_n(newVertices->data(), newVertices->dataSize(), 0);

	const std::size_t oldVertexSize = oldVertexDescription.getVertexSize();
	const std::size_t newVertexSize = newVertexDescription.getVertexSize();
	for(const auto & oldAttr : oldVertexDescription.getAttributes()) {
		const VertexAttribute & newAttr = newVertexDescription.getAttribute(oldAttr.getNameId());

		if (oldAttr.empty() || newAttr.empty()) {
			continue;
		}
		if(oldAttr.getDataType() == newAttr.getDataType()) {					
			uint32_t dataSize = std::min(oldAttr.getDataSize(), newAttr.getDataSize());
			const uint8_t * source = oldVertices.data() + oldAttr.getOffset();
			uint8_t * target = newVertices->data() + newAttr.getOffset();
			for (uint32_t i = 0; i < numVertices; ++i) {
				std::copy(source, source + dataSize, target);
				source += oldVertexSize;
				target += newVertexSize;
			}			
		} else if( canConvert(oldAttr, newAttr) ) {
			auto oldAcc = FloatAttributeAccessor::create(const_cast<MeshVertexData&>(oldVertices), newAttr.getNameId());
			auto newAcc = FloatAttributeAccessor::create(*newVertices, newAttr.getNameId());
			for (uint32_t i = 0; i < numVertices; ++i) {
				newAcc->setValues(i, oldAcc->getValues(i));
			}
		}
	}
	newVertices->updateBoundingBox();
	return newVertices;
}

// -----------------------------------------------------------------------------

VertexDescription uniteVertexDescriptions(const std::deque<VertexDescription> & vertexDescs) {
	VertexDescription result;
	for(const auto & desc : vertexDescs) {
		const auto & attributes = desc.getAttributes();
		for(const auto & attr : attributes) {
			const VertexAttribute & resultAttr = result.getAttribute(attr.getNameId());
			if (resultAttr.empty()) {
				result.appendAttribute(attr.getNameId(), attr.getDataType(), attr.getNumValues(), attr.isNormalized());
			} else if (!(attr == resultAttr)) {
				uint8_t attrTypeSize = Util::getNumBytes(attr.getDataType());
				uint8_t resultAttrTypeSize = Util::getNumBytes(resultAttr.getDataType());
				result.updateAttribute(
						VertexAttribute(attr.getNameId(), attrTypeSize > resultAttrTypeSize ? attr.getDataType() : resultAttr.getDataType(), 
							std::max(attr.getNumValues(), resultAttr.getNumValues()), attr.isNormalized()&&resultAttr.isNormalized() ));
			}
		}
	}
	return result;
}

// -----------------------------------------------------------------------------

//! (static)
void removeColorData(Mesh * m) {
	MeshVertexData & vertices = m->openVertexData();
	const VertexDescription & vdo = vertices.getVertexDescription();
	VertexDescription vdn = vdo;
	vdn.appendAttribute(VertexAttributeIds::COLOR, Util::TypeConstant::UINT8, 0, false);

	std::unique_ptr<MeshVertexData> newVertices(convertVertices(vertices, vdn));
	vertices.swap(*newVertices.get());
}

// -----------------------------------------------------------------------------

//! (static)
void calculateNormals(Mesh * m) {
	MeshVertexData & vData = m->openVertexData();

	// add normals to vData if necessary
	if (!vData.getVertexDescription().hasAttribute(VertexAttributeIds::NORMAL)) {
		VertexDescription newVd = vData.getVertexDescription();
		newVd.appendNormalByte();
		std::unique_ptr<MeshVertexData> newVertices(convertVertices(vData, newVd));
		vData.swap(*newVertices.get());
	}

	const uint32_t vertexCount = vData.getVertexCount();

	std::vector<Geometry::Vec3> normals(vertexCount);

	// accumulate normals
	const MeshIndexData & indices = m->openIndexData();
	const uint32_t indexCount = m->getIndexCount();
	Util::Reference<PositionAttributeAccessor> positionAccessor(PositionAttributeAccessor::create(vData,VertexAttributeIds::POSITION));
	for (uint32_t i = 0; i < indexCount-2; i += 3) {
		// n = cb x ab
		const Geometry::Vec3 a( positionAccessor->getPosition(indices[i + 0]) );
		const Geometry::Vec3 b( positionAccessor->getPosition(indices[i + 1]) );
		const Geometry::Vec3 c( positionAccessor->getPosition(indices[i + 2]) );

		Geometry::Vec3 n( (c-b).cross(a-b) );
		const float length = n.length();
		if(length>0)
			n.normalize();
		normals[indices[i+0]] += n;
		normals[indices[i+1]] += n;
		normals[indices[i+2]] += n;
	}

	// set normals
	Util::Reference<NormalAttributeAccessor> normalAccessor(NormalAttributeAccessor::create(vData,VertexAttributeIds::NORMAL));
	for(uint32_t i=0;i<vertexCount;++i){
		const Geometry::Vec3 n = normals[i];
		const float length = n.length();
		normalAccessor->setNormal(i, length>0 ? n/length : n);
	}
	vData.markAsChanged();
}

// -----------------------------------------------------------------------------

/**
 * [static]
 * Combines the meshes from meshArray to a single mesh.
 * Must have identical VertexDescription.
 */
Mesh * combineMeshes(const std::deque<Mesh *> & meshArray) {
	return combineMeshes(meshArray, std::deque<Geometry::Matrix4x4>());
}

// -----------------------------------------------------------------------------

Mesh * combineMeshes(const std::deque<Mesh *> & meshArray, const std::deque<Geometry::Matrix4x4> & transformations) {
	if (meshArray.empty()) {
		return nullptr;
	}

	Mesh * firstMesh = meshArray.front();
	if (!firstMesh)
		FAIL();

	const VertexDescription & vd = firstMesh->getVertexDescription();

	std::deque<Mesh *> meshArray2; //< collect meshes which have the same vertexDescription
	std::deque<Geometry::Matrix4x4> transformations2; //< collect meshes which have the same vertexDescription

	//count vertices and indices, check if Meshes exist and have the same vertexDescription
	uint32_t indexCount = 0;
	uint32_t vertexCount = 0;
	{
		auto tIt = transformations.begin();
		for (auto it = meshArray.begin(); it != meshArray.end(); ++it, (tIt != transformations.end() ? ++tIt : tIt)) {
			if (!(*it)) {
				WARN("combineMeshes: No Mesh");
				continue;
			}
			if (!((*it)->getVertexDescription() == vd)) {
				WARN("combineMeshes: can't combine meshes with different vertex descriptions.");
				std::cout << (*it)->getVertexDescription().toString() << ":" << vd.toString() << "\n";
				continue;
			}
			meshArray2.push_back(*it);
			if (tIt != transformations.end())
				transformations2.push_back(*tIt);
			indexCount += (*it)->getIndexCount();
			vertexCount += (*it)->getVertexCount();
		}
	}
	// create mesh
	auto mesh = new Mesh;
	MeshVertexData & vertices = mesh->openVertexData();
	vertices.allocate(vertexCount, vd);
	MeshIndexData & indices = mesh->openIndexData();
	indices.allocate(indexCount);

	// copy data

	uint32_t indexPointer = 0;
	uint32_t vertexPointer = 0;

	Geometry::Matrix4x4 noTrans;
	auto tIt2 = transformations2.cbegin();
	for (auto it = meshArray2.cbegin(); it != meshArray2.cend(); ++it, (tIt2 != transformations2.cend() ? ++tIt2 : tIt2)) {
		Mesh * currentMesh = *it;
		// add modified indices
		MeshIndexData & currentIndices = currentMesh->openIndexData();
		for (uint32_t j = 0; j < currentIndices.getIndexCount(); ++j)
			indices[indexPointer + j] = currentIndices[j] + vertexPointer;
		indexPointer += currentIndices.getIndexCount();

		// add vertices
		MeshVertexData & currentVertices = currentMesh->openVertexData();
		std::copy(currentVertices.data(), currentVertices.data() + currentVertices.dataSize(), vertices[vertexPointer]);

		if (tIt2 != transformations2.cend() && (*tIt2) != noTrans) {
			transformVertexData(vertices, (*tIt2), vertexPointer, currentVertices.getVertexCount());
		}
		vertexPointer += currentVertices.getVertexCount();
	}
	vertices.updateBoundingBox();
	indices.updateIndexRange();

	return mesh;
}

// -----------------------------------------------------------------------------

/**
 * [static]
 * Splits a meshs vertex data into several data chunks of the given size.
 */

std::deque<MeshVertexData> splitVertexData(Mesh * mesh, uint32_t chunkSize){
	std::deque<MeshVertexData> result;

	const VertexDescription & desc = mesh->getVertexDescription();
	MeshVertexData & meshVertices = mesh->openVertexData();
	uint32_t vertexCount = mesh->getVertexCount();

	uint32_t vertexPointer = 0;

	while (vertexPointer < vertexCount){
		uint32_t currentChunkSize;
		if (vertexCount > vertexPointer + chunkSize)
			currentChunkSize = chunkSize;
		else
			currentChunkSize = vertexCount - vertexPointer;

		MeshVertexData currentVertices;
		currentVertices.allocate(currentChunkSize, desc);

		uint32_t chunkFront = vertexPointer * desc.getVertexSize();
		uint32_t chunkEnd = chunkFront + currentChunkSize * desc.getVertexSize();


		std::copy(meshVertices.data() + chunkFront, meshVertices.data() + chunkEnd, currentVertices.data());

		result.emplace_back(std::move(currentVertices));

		vertexPointer += chunkSize;
	}

	return result;
}

// -----------------------------------------------------------------------------

/**
 * [static]
 * Extracts a range of vertices from a mesh.
 */
MeshVertexData * extractVertexData(Mesh * mesh, uint32_t begin, uint32_t length){

	const VertexDescription & desc = mesh->getVertexDescription();
	MeshVertexData & meshVertices = mesh->openVertexData();
	uint32_t vertexCount = mesh->getVertexCount();

	if (begin+length > vertexCount)
		return nullptr;

	auto result = new MeshVertexData;
	result->allocate(length, desc);

	uint32_t front = begin*desc.getVertexSize();
	uint32_t end = front + length*desc.getVertexSize();

	std::copy(meshVertices.data() + front, meshVertices.data() + end, result->data());

	return result;

}

// -----------------------------------------------------------------------------

//! (static)
void eliminateDuplicateVertices(Mesh * mesh) {
	const VertexDescription & desc = mesh->getVertexDescription();
	const uint32_t indexCount = mesh->getIndexCount();

	// Set of vertices used to create unique vertices.
	std::set<RawVertex> rawVertices;
	// Mapping from old index to new index.
	std::map<uint32_t, uint32_t> indexReplace;

	// Simply go over the indices and add one vertex after another.
	{
		const MeshVertexData & vertices = mesh->openVertexData();
		const MeshIndexData & indices = mesh->openIndexData();
		const std::size_t vertexSize = desc.getVertexSize();

		for (uint32_t counter = 0; counter < indexCount; ++counter) {
			const uint32_t index = indices[counter];
			const uint8_t * vertex = vertices[index];
			RawVertex raw(index, vertex, vertexSize);
			auto it = rawVertices.insert(raw).first;
			indexReplace.insert(std::make_pair(index, it->getIndex()));
		}
	}

	// Mapping from new index to vertex position.
	std::map<uint32_t, uint32_t> indexPosition;

	// Create the new mesh and add the rawVertices.
	Util::Reference<Mesh> result = new Mesh;
	result->setDataStrategy(mesh->getDataStrategy());

	MeshVertexData & vertices = result->openVertexData();
	vertices.allocate(rawVertices.size(), desc);

	MeshIndexData & indices = result->openIndexData();
	indices.allocate(indexCount);

	{
		uint8_t * data = vertices.data();
		uint32_t vertexPos = 0;
		for(const auto & vertex : rawVertices) {
			std::copy(vertex.getData(), vertex.getData() + vertex.getSize(), data);
			data += vertex.getSize();
			indexPosition.insert(std::make_pair(vertex.getIndex(), vertexPos));
			++vertexPos;
		}
		// Translate the indices.
		const uint32_t * srcIndex = mesh->openIndexData().data();
		uint32_t * dstIndex = indices.data();
		for (uint32_t counter = 0; counter < indexCount; ++counter) {
			const uint32_t newIndex = indexReplace.find(*srcIndex)->second;
			*dstIndex = indexPosition.find(newIndex)->second;
			++srcIndex;
			++dstIndex;
		}
	}

	vertices.updateBoundingBox();
	indices.updateIndexRange();

	mesh->swap(*result.get());
}

// -----------------------------------------------------------------------------

//! (static)
Mesh * eliminateUnusedVertices(Mesh * mesh) {
	const VertexDescription & desc = mesh->getVertexDescription();

	const uint32_t indexCount = mesh->getIndexCount();
	const MeshIndexData & indices = mesh->openIndexData();

	// Mapping from old index to new index.
	static const uint32_t NONE = 0xffffffff;
	//std::vector<uint32_t> oldToNewIndices(mesh->getVertexCount(), NONE);
	std::unordered_map<uint32_t,uint32_t> oldToNewIndices;
	oldToNewIndices.reserve(indexCount);
	std::vector<uint32_t> usedOldVertices;
	//usedOldVertices.reserve(mesh->getVertexCount());
	usedOldVertices.reserve(std::min(indexCount, mesh->getVertexCount()));
	std::vector<uint32_t> newIndices;
	newIndices.reserve(indexCount);

	for (uint32_t counter = 0; counter < indexCount; ++counter) {
		const uint32_t & oldIndex = indices[counter];
		auto it = oldToNewIndices.find(oldIndex);		
		uint32_t newIndex = NONE;
		if (it == oldToNewIndices.end()) {
			newIndex = usedOldVertices.size();
			usedOldVertices.push_back(oldIndex);
			oldToNewIndices[oldIndex] = newIndex;
		} else {
			newIndex = it->second;
		}
		newIndices.push_back(newIndex);
	}

	auto newMesh = new Mesh(desc, usedOldVertices.size(), newIndices.size());
	MeshIndexData & newIndexData = newMesh->openIndexData();
	std::copy(newIndices.begin(), newIndices.end(), newIndexData.data());
	newIndexData.updateIndexRange();

	const MeshVertexData & oldVertexData = mesh->openVertexData();
	MeshVertexData & newVertexData = newMesh->openVertexData();
	const size_t vSize = desc.getVertexSize();
	uint32_t i = 0;
	for(const auto & oldIndex : usedOldVertices) {
		std::copy(oldVertexData[oldIndex], oldVertexData[oldIndex] + vSize, newVertexData[i++]);
	}
	newVertexData.updateBoundingBox();

	return newMesh;
}

// -----------------------------------------------------------------------------

//! (static)
Mesh * eliminateLongTriangles(Mesh * mesh, float ratio) {
	const MeshIndexData & originalIndices = mesh->openIndexData();
	const MeshVertexData & vertexData = mesh->openVertexData();
	std::deque<uint32_t> newIndices;
	const uint32_t indexCount = mesh->getIndexCount();

	for (uint32_t counter = 0; counter < indexCount; counter += 3) {
		Geometry::Vec3 p1(reinterpret_cast<const float*> (vertexData[originalIndices[counter]]));
		Geometry::Vec3 p2(reinterpret_cast<const float*> (vertexData[originalIndices[counter + 1]]));
		Geometry::Vec3 p3(reinterpret_cast<const float*> (vertexData[originalIndices[counter + 2]]));

		float a2 = (p1 - p2).lengthSquared();
		float b2 = (p2 - p3).lengthSquared();
		float c2 = (p1 - p3).lengthSquared();

		if (a2 == 0.0f || b2 == 0.0f || c2 == 0.0f)
			continue;

		float f = sqrtf(2.0f * (a2 * b2 + b2 * c2 + c2 * a2) - (a2 * a2 + b2 * b2 + c2 * c2));

		float max = sqrtf(a2 > b2 ? (a2 > c2 ? a2 : c2) : (b2 > c2 ? b2 : c2));
		// maximum height of the triangle
		float h_max = f / (2.0f * max);

		if (max > ratio * h_max)
			continue;
		newIndices.push_back(originalIndices[counter]);
		newIndices.push_back(originalIndices[counter + 1]);
		newIndices.push_back(originalIndices[counter + 2]);
	}
	MeshIndexData newIndexData;
	newIndexData.allocate(newIndices.size());
	std::copy(newIndices.begin(), newIndices.end(), newIndexData.data());
	Util::Reference<Mesh> newMesh = new Mesh(newIndexData, vertexData);
	return eliminateUnusedVertices(newMesh.get());
}

// -----------------------------------------------------------------------------

Mesh * eliminateTrianglesBehindPlane(Mesh * mesh, const Geometry::Plane & plane) {
	const MeshIndexData & originalIndices = mesh->openIndexData();
	const MeshVertexData & vertexData = mesh->openVertexData();
	std::deque<uint32_t> newIndices;
	const uint32_t indexCount = mesh->getIndexCount();

	for (uint_fast32_t counter = 0; counter < indexCount; counter += 3) {
		const uint32_t & indexA = originalIndices[counter];
		const uint32_t & indexB = originalIndices[counter + 1];
		const uint32_t & indexC = originalIndices[counter + 2];
		{
			const Geometry::Vec3 vertex(reinterpret_cast<const float *> (vertexData[indexA]));
			if (plane.planeTest(vertex) < 0.0f) {
				continue;
			}
		}
		{
			const Geometry::Vec3 vertex(reinterpret_cast<const float *> (vertexData[indexB]));
			if (plane.planeTest(vertex) < 0.0f) {
				continue;
			}
		}
		{
			const Geometry::Vec3 vertex(reinterpret_cast<const float *> (vertexData[indexC]));
			if (plane.planeTest(vertex) < 0.0f) {
				continue;
			}
		}
		newIndices.push_back(indexA);
		newIndices.push_back(indexB);
		newIndices.push_back(indexC);
	}
	MeshIndexData newIndexData;
	newIndexData.allocate(newIndices.size());
	std::copy(newIndices.begin(), newIndices.end(), newIndexData.data());
	newIndexData.updateIndexRange();
	return new Mesh(newIndexData, vertexData);
}

// -----------------------------------------------------------------------------

Mesh * eliminateZeroAreaTriangles(Mesh * mesh) {
	const MeshIndexData & originalIndices = mesh->openIndexData();
	const MeshVertexData & vertexData = mesh->openVertexData();
	const uint32_t indexCount = mesh->getIndexCount();
	std::vector<uint32_t> newIndices;
	newIndices.reserve(indexCount);

	for (uint_fast32_t counter = 0; counter < indexCount; counter += 3) {
		const uint32_t & indexA = originalIndices[counter];
		const uint32_t & indexB = originalIndices[counter + 1];
		const uint32_t & indexC = originalIndices[counter + 2];
		const Geometry::Triangle<Geometry::Vec3f> triangle(
						Geometry::Vec3f(reinterpret_cast<const float *>(vertexData[indexA])),
						Geometry::Vec3f(reinterpret_cast<const float *>(vertexData[indexB])),
						Geometry::Vec3f(reinterpret_cast<const float *>(vertexData[indexC])));

		if (!triangle.isDegenerate()) {
			newIndices.push_back(indexA);
			newIndices.push_back(indexB);
			newIndices.push_back(indexC);
		}
	}
	MeshIndexData newIndexData;
	newIndexData.allocate(newIndices.size());
	std::copy(newIndices.begin(), newIndices.end(), newIndexData.data());
	newIndexData.updateIndexRange();
	return new Mesh(newIndexData, vertexData);
}

// -----------------------------------------------------------------------------

static void normalize(float * n) {
	const float length = std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
	static int bla = 0;
	if (length == 0.0f) {
		std::cout << "\r null normal " << bla++;
		return;
	}
	const float factor = 1.0f / length;
	n[0] *= factor;
	n[1] *= factor;
	n[2] *= factor;
}

// -----------------------------------------------------------------------------

static void calcNormal(const float * a, const float * b, const float * c, float * n) {
	/*  a_____________c
	 *   ^           ^
	 *    \    n    /
	 *     \   ^   /
	 *   ab \  |  / cb
	 *       \ | /
	 *        \|/
	 *         b
	 */
	const float ab[3] = { a[0] - b[0], a[1] - b[1], a[2] - b[2] };
	const float cb[3] = { c[0] - b[0], c[1] - b[1], c[2] - b[2] };

	// n = cb x ab
	n[0] = cb[1] * ab[2] - cb[2] * ab[1];
	n[1] = cb[2] * ab[0] - cb[0] * ab[2];
	n[2] = cb[0] * ab[1] - cb[1] * ab[0];

	normalize(n);
}

// -----------------------------------------------------------------------------

Mesh * removeSkinsWithHoleCovering(Mesh * mesh, float maxNormalZ, float coveringMovement) {
	const uint32_t indexCount = mesh->getIndexCount();
	const MeshIndexData & originalIndices = mesh->openIndexData();
	std::deque<uint32_t> newIndices;
	const MeshVertexData & vertexData = mesh->openVertexData();
	MeshVertexData newVertexData = vertexData;

	for (uint32_t counter = 0; counter < indexCount; counter += 3) {
		const uint32_t & indexA = originalIndices[counter];
		const uint32_t & indexB = originalIndices[counter + 1];
		const uint32_t & indexC = originalIndices[counter + 2];
		const float * vertexA = reinterpret_cast<const float *> (vertexData[indexA]);
		const float * vertexB = reinterpret_cast<const float *> (vertexData[indexB]);
		const float * vertexC = reinterpret_cast<const float *> (vertexData[indexC]);

		float normal[3];
		calcNormal(vertexA, vertexB, vertexC, normal);

		if (std::abs(normal[2]) <= maxNormalZ) {
			// Calculate movement direction by normal calculation.
			const float minZ = std::min(std::min(vertexA[2], vertexB[2]), vertexC[2]);
			const float maxZ = std::max(std::max(vertexA[2], vertexB[2]), vertexC[2]);
			const float depthRange = (maxZ - minZ);

			// Move the vertices lying in the background.
			const float halfZ = (maxZ + minZ) / 2.0f;
			if (vertexA[2] > halfZ) {
				float * newVertexA = reinterpret_cast<float *> (newVertexData[indexA]);
				newVertexA[0] += coveringMovement * depthRange * normal[0];
				newVertexA[1] += coveringMovement * depthRange * normal[1];
			}
			if (vertexB[2] > halfZ) {
				float * newVertexB = reinterpret_cast<float *> (newVertexData[indexB]);
				newVertexB[0] += coveringMovement * depthRange * normal[0];
				newVertexB[1] += coveringMovement * depthRange * normal[1];
			}
			if (vertexC[2] > halfZ) {
				float * newVertexC = reinterpret_cast<float *> (newVertexData[indexC]);
				newVertexC[0] += coveringMovement * depthRange * normal[0];
				newVertexC[1] += coveringMovement * depthRange * normal[1];
			}
			// Remove face by not inserting its indices.
			continue;
		}
		newIndices.push_back(originalIndices[counter]);
		newIndices.push_back(originalIndices[counter + 1]);
		newIndices.push_back(originalIndices[counter + 2]);
	}
	MeshIndexData newIndexData;
	newIndexData.allocate(newIndices.size());
	std::copy(newIndices.begin(), newIndices.end(), newIndexData.data());
	newIndexData.updateIndexRange();
	newVertexData.updateBoundingBox();
	return new Mesh(newIndexData, newVertexData);
}

// -----------------------------------------------------------------------------

void optimizeIndices(Mesh * mesh, const uint_fast8_t _cacheSize) {
	struct Inner {
		/**
		 * Consider all 1-ring candidates and select the best for fanning.
		 *
		 * @param stop Flag which tells if the algorithm has ended.
		 * @param nextCand Passed from caller.
		 * @param stamp Passed from caller.
		 * @param cacheSize Passed from caller.
		 * @param cacheTimes Passed from caller.
		 * @param liveTriangles Passed from caller.
		 * @param deadEndStack Passed from caller.
		 * @param numVertices Passed from caller.
		 * @param cursor Passed from caller.
		 * @return Number of next vertex.
		 */
		static uint32_t getNextVertex(bool & stop, const std::set<uint32_t> & nextCand, const uint32_t stamp, const uint8_t cacheSize,
				const uint32_t * cacheTimes, const uint_fast16_t * liveTriangles, std::stack<uint32_t> & deadEndStack, const uint32_t numVertices,
				uint32_t & cursor) {
			// Best candidate and priority.
			bool found = false;
			uint32_t n = 0;
			uint32_t maxPriority = 0; // m
			for(const auto & v : nextCand) {
				// Must have live triangles.
				if (liveTriangles[v] > 0) {
					// Initial priority.
					uint32_t p = 0;
					// In cache even after fanning?
					if (stamp - cacheTimes[v] + 2 * liveTriangles[v] <= cacheSize) {
						// Priority is position in cache.
						p = stamp - cacheTimes[v];
					}
					// Keep best candidate.
					if (p > maxPriority) {
						maxPriority = p;
						n = v;
						found = true;
					}
				}
			}
			// Reached a dead-end?
			if (!found) {
				n = skipDeadEnd(stop, liveTriangles, deadEndStack, numVertices, cursor);
			}
			return n;
		}

		/**
		 * When there are no candidates left we might have to choose an non-
		 * local vertex.
		 *
		 * @param stop Flag which tells if the algorithm has ended.
		 * @param liveTriangles Passed from caller.
		 * @param deadEndStack Passed from caller.
		 * @param numVertices Passed from caller.
		 * @param cursor Passed from caller.
		 * @return Number of non-local vertex.
		 */
		static uint32_t skipDeadEnd(bool & stop, const uint_fast16_t * liveTriangles, std::stack<uint32_t> & deadEndStack,
				const uint32_t numVertices, uint32_t & cursor) {
			while (!deadEndStack.empty()) {
				// Next in dead-end stack.
				uint32_t d = deadEndStack.top();
				deadEndStack.pop();
				// Check for live triangles.
				if (liveTriangles[d] > 0) {
					return d;
				}
			}
			while (cursor < numVertices) {
				// Check for live triangles.
				if (liveTriangles[cursor] > 0) {
					return cursor;
				}
				// Next in input order.
				// Cursor sweeps list only once.
				++cursor;
			}
			// We are done!
			stop = true;
			return 0;
		}
	};
	if (mesh->getDrawMode() != Mesh::DRAW_TRIANGLES) {
		WARN("This function only works with meshes with a triangle list.");
		return;
	}
	uint32_t numVertices = mesh->getVertexCount();
	uint32_t numIndices = mesh->getIndexCount();
	uint32_t numTriangles = numIndices / 3;
	MeshIndexData & indices = mesh->openIndexData();

	// Build vertex-triangle adjacency.
	// First pass: Count occurrences.
	auto occurrences = new uint_fast16_t[numVertices];
	std::fill_n(occurrences, numVertices, 0);
	for (uint32_t i = 0; i < numIndices; ++i) {
		++occurrences[indices[i]];
	}
	// Second pass: Create the offset map.
	auto offsetMap = new uint32_t[numVertices];
	uint32_t sum = 0;
	for (uint32_t v = 0; v < numVertices; ++v) {
		offsetMap[v] = sum;
		sum += occurrences[v];
	}
	// Third pass: Construct triangle lists.
	auto triangleLists = new uint32_t[sum]; // A
	auto tmpOffsetMap = new uint32_t[numVertices];
	std::copy(offsetMap, offsetMap + numVertices, tmpOffsetMap);
	for (uint32_t i = 0; i < numIndices; ++i) {
		uint32_t vertex = indices[i];
		uint32_t offset = tmpOffsetMap[vertex];
		triangleLists[offset] = i / 3;
		++tmpOffsetMap[vertex];
	}
	delete[] tmpOffsetMap;

	// Create per-vertex live triangle count.
	auto liveTriangles = new uint_fast16_t[numVertices]; // L
	std::copy(occurrences, occurrences + numVertices, liveTriangles);
	// Create per-vertex caching time stamps.
	auto cacheTimes = new uint32_t[numVertices]; // C
	std::fill_n(cacheTimes, numVertices, 0);
	// Create dead-end vertex stack.
	std::stack<uint32_t> deadEndStack; // D
	// Create per-triangles emitted flags.
	auto emitted = new bool[numTriangles]; // E
	for (uint32_t t = 0; t < numTriangles; ++t) {
		emitted[t] = false;
	}
	// Create output buffer.
	MeshIndexData newIndices;
	newIndices.allocate(numIndices);
	uint32_t * output = newIndices.data();
	// Initialize fanning vertex.
	uint32_t fanVertex = 0; // f
	// Initialize time stamp.
	uint32_t stamp = static_cast<uint32_t> (_cacheSize) + 1; // s
	// Initialize the cursor.
	uint32_t cursor = 1; // i

	bool stop = false;
	while (!stop) {
		// 1-ring of next candidates.
		std::set<uint32_t> nextCand; // N
		uint_fast16_t numNeighbors = occurrences[fanVertex];
		for (uint_fast16_t i = 0; i < numNeighbors; ++i) {
			uint32_t t = triangleLists[offsetMap[fanVertex] + i];
			if (emitted[t])
				continue;

			for (uint_fast8_t ii = 0; ii < 3; ++ii) {
				uint32_t v = indices[3 * t + ii];
				// Output vertex.
				*output = v;
				++output;
				// Add to dead-end stack.
				deadEndStack.push(v);
				// Register as candidate.
				nextCand.insert(v);
				// Decrease live triangle count.
				--liveTriangles[v];
				// If not in cache
				if (stamp - cacheTimes[v] > _cacheSize) {
					// Set time stamp.
					cacheTimes[v] = stamp;
					// Increment time stamp.
					++stamp;
				}
			}
			// Flag triangle as emitted.
			emitted[t] = true;

		}
		fanVertex = Inner::getNextVertex(stop, nextCand, stamp, _cacheSize, cacheTimes, liveTriangles, deadEndStack, numVertices, cursor);
	}

	// Clean up.
	delete[] occurrences;
	delete[] offsetMap;
	delete[] triangleLists;
	delete[] liveTriangles;
	delete[] cacheTimes;
	delete[] emitted;

	newIndices.markAsChanged();
	newIndices.updateIndexRange();

	// Modify the mesh.
	indices.swap(newIndices);
}

// -----------------------------------------------------------------------------

void reverseWinding(Mesh * mesh) {
	if (mesh->getDrawMode() != Mesh::DRAW_TRIANGLES) {
		WARN("TRIANGLES is the only supported mode.");
		return;
	}
	MeshIndexData & id = mesh->openIndexData();
	uint32_t * indices = id.data();
	for (uint32_t i = 0; i < mesh->getIndexCount(); i += 3) {
		uint32_t temp = indices[i];
		indices[i] = indices[i + 2];
		indices[i + 2] = temp;
	}
	id.markAsChanged();
}

// -----------------------------------------------------------------------------

//! (static)
void copyVertexAttribute(Mesh * mesh, Util::StringIdentifier from, Util::StringIdentifier to) {
	MeshVertexData & vertices = mesh->openVertexData();

	if (vertices.getVertexDescription().getAttribute(from).empty()) {
		WARN("Source data not available.");
		return;
	}
	{
		VertexDescription vdCopy(vertices.getVertexDescription());
		const VertexAttribute & vaFrom = vdCopy.getAttribute(from);
		VertexAttribute vaTo(to, vaFrom.getDataType(), vaFrom.getNumValues(), vaFrom.isNormalized());
		vdCopy.updateAttribute(vaTo);

		std::unique_ptr<MeshVertexData> newVertices(convertVertices(vertices, vdCopy));
		vertices.swap(*newVertices.get());
	}

	const VertexDescription & vd = vertices.getVertexDescription();
	const VertexAttribute & vaFrom = vd.getAttribute(from);
	const VertexAttribute & vaTo = vd.getAttribute(to);

	uint8_t * data = vertices.data();
	const size_t stride = vd.getVertexSize();
	const uint16_t offsetFrom = vaFrom.getOffset();
	const uint16_t offsetTo = vaTo.getOffset();
	const size_t attrSize = vaFrom.getDataSize();

	for (uint_fast32_t v = 0; v < vertices.getVertexCount(); ++v) {
		std::copy(data + offsetFrom , data + offsetFrom + attrSize, data + offsetTo);
		data += stride;
	}

	vertices.markAsChanged();
}

// -----------------------------------------------------------------------------

//! (static)
void calculateTextureCoordinates_projection(Mesh * mesh, Util::StringIdentifier attribName, const Geometry::Matrix4x4 & projection) {
	MeshVertexData & vData = mesh->openVertexData();

	// add slot for 2 float coordinates
	if (!vData.getVertexDescription().hasAttribute(attribName)) {
		VertexDescription newVd = vData.getVertexDescription();
		newVd.appendFloat(attribName, 2, false);
		std::unique_ptr<MeshVertexData> newVertices(convertVertices(vData, newVd));
		vData.swap(*newVertices.get());
	}

	Util::Reference<PositionAttributeAccessor> positionAccessor(PositionAttributeAccessor::create(vData,VertexAttributeIds::POSITION));
	Util::Reference<TexCoordAttributeAccessor> texCoordAccessor(TexCoordAttributeAccessor::create(vData,attribName));

	for(uint32_t i=0;texCoordAccessor->checkRange(i);++i){
		const Geometry::Vec3 v( projection.transformPosition(positionAccessor->getPosition(i)));
		texCoordAccessor->setCoordinate(i,Geometry::Vec2(v.x(),v.y()));
	}

	vData.markAsChanged();
}

// -----------------------------------------------------------------------------

//!	(static)
void calculateTangentVectors(Mesh * mesh, const Util::StringIdentifier uvName, const Util::StringIdentifier tangentVecName) {
	using Geometry::Vec3;
	using Geometry::Vec2;
	MeshVertexData & vertices(mesh->openVertexData());
	const MeshIndexData & indices(mesh->openIndexData());

	{ // assure mesh has the right form
		if (mesh->getDrawMode() != Mesh::DRAW_TRIANGLES)
			INVALID_ARGUMENT_EXCEPTION("addTangentVectors: No triangle mesh.");

		if (vertices.getVertexDescription().getAttribute(VertexAttributeIds::POSITION).getDataType() != Util::TypeConstant::FLOAT)
			INVALID_ARGUMENT_EXCEPTION("addTangentVectors: No float positions.");

		if (vertices.getVertexDescription().getAttribute(VertexAttributeIds::NORMAL).empty())
			INVALID_ARGUMENT_EXCEPTION("addTangentVectors: No normals.");

		if (vertices.getVertexDescription().getAttribute(uvName).getDataType() != Util::TypeConstant::FLOAT
				|| vertices.getVertexDescription().getAttribute(uvName).getNumValues() < 2)
			INVALID_ARGUMENT_EXCEPTION("addTangentVectors: No or wrong texture coordinates.");

		// add slot for 4 byte tangent vector
		if (vertices.getVertexDescription().getAttribute(tangentVecName).empty()) {
			VertexDescription newVd = vertices.getVertexDescription();
			newVd.appendAttribute(tangentVecName, Util::TypeConstant::INT8, 4, true);
			std::unique_ptr<MeshVertexData> newVertices(convertVertices(vertices, newVd));
			vertices.swap(*newVertices.get());
		}
		if (vertices.getVertexDescription().getAttribute(tangentVecName).getDataType() != Util::TypeConstant::INT8 || vertices.getVertexDescription().getAttribute(
				tangentVecName).getNumValues() != 4)
			INVALID_ARGUMENT_EXCEPTION("createTextureCoordinates_boxProjection: Wrong tangent format.");

	}
	// calculate tangents and bitangents
	const VertexDescription & vDesc = vertices.getVertexDescription();
	const VertexAttribute & posAttr = vDesc.getAttribute(VertexAttributeIds::POSITION);
	const VertexAttribute & normalAttr = vDesc.getAttribute(VertexAttributeIds::NORMAL);
	const VertexAttribute & uvAttr = vDesc.getAttribute(uvName);
	const VertexAttribute & tanAttr = vDesc.getAttribute(tangentVecName);

	std::vector<Vec3> tan1(vertices.getVertexCount());
	std::vector<Vec3> tan2(vertices.getVertexCount());

	for (uint32_t i = 0; i < indices.getIndexCount(); i += 3) {
		const uint32_t index1 = indices[i];
		const uint32_t index2 = indices[i + 1];
		const uint32_t index3 = indices[i + 2];

		const Vec3 pos1(reinterpret_cast<float*> (vertices[index1] + posAttr.getOffset()));
		const Vec3 pos2(reinterpret_cast<float*> (vertices[index2] + posAttr.getOffset()));
		const Vec3 pos3(reinterpret_cast<float*> (vertices[index3] + posAttr.getOffset()));

		const Vec2 uv1(reinterpret_cast<float*> (vertices[index1] + uvAttr.getOffset()));
		const Vec2 uv2(reinterpret_cast<float*> (vertices[index2] + uvAttr.getOffset()));
		const Vec2 uv3(reinterpret_cast<float*> (vertices[index3] + uvAttr.getOffset()));

		const float x1 = pos2.x() - pos1.x();
		const float x2 = pos3.x() - pos1.x();
		const float y1 = pos2.y() - pos1.y();
		const float y2 = pos3.y() - pos1.y();
		const float z1 = pos2.z() - pos1.z();
		const float z2 = pos3.z() - pos1.z();

		const float s1 = uv2.x() - uv1.x();
		const float s2 = uv3.x() - uv1.x();
		const float t1 = uv2.y() - uv1.y();
		const float t2 = uv3.y() - uv1.y();

		const float r = 1.0f / (s1 * t2 - s2 * t1);
		const Vec3 sdir((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
		const Vec3 tdir((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);

		tan1[index1] += sdir;
		tan1[index2] += sdir;
		tan1[index3] += sdir;

		tan2[index1] += tdir;
		tan2[index2] += tdir;
		tan2[index3] += tdir;
	}

	// -----------------------------------------------------------------------------

	if (normalAttr.getDataType() == Util::TypeConstant::FLOAT) {
		for (uint32_t i = 0; i < vertices.getVertexCount(); ++i) {
			const Vec3 normal(reinterpret_cast<float*> (vertices[i] + normalAttr.getOffset()));
			const Vec3 & t = tan1[i];
			const Vec3 tan((t - normal * normal.dot(t)).getNormalized() * 127); // Gram-Schmidt orthogonalize

			int8_t * const tPtr = reinterpret_cast<int8_t*> (vertices[i] + tanAttr.getOffset());
			int8_t handedness = (normal.cross(t).dot(tan2[i]) < 0.0f) ? -1 : 1; // Calculate handedness
			tPtr[0] = handedness * static_cast<int8_t> (tan.x());
			tPtr[1] = handedness * static_cast<int8_t> (tan.y());
			tPtr[2] = handedness * static_cast<int8_t> (tan.z());
			tPtr[3] = handedness;
		}
	} else if (normalAttr.getDataType() == Util::TypeConstant::INT8) {
		for (uint32_t i = 0; i < vertices.getVertexCount(); ++i) {
			const int8_t * nPtr = reinterpret_cast<const int8_t*> (vertices[i] + normalAttr.getOffset());
			const Vec3 normal((Vec3(nPtr[0], nPtr[1], nPtr[2])).normalize());
			const Vec3 & t = tan1[i];
			const Vec3 tan((t - normal * normal.dot(t)).getNormalized() * 127); // Gram-Schmidt orthogonalize

			int8_t * const tPtr = reinterpret_cast<int8_t*> (vertices[i] + tanAttr.getOffset());
			int8_t handedness = (normal.cross(t).dot(tan2[i]) < 0.0f) ? -1 : 1; // Calculate handedness
			tPtr[0] = handedness * static_cast<int8_t> (tan.x());
			tPtr[1] = handedness * static_cast<int8_t> (tan.y());
			tPtr[2] = handedness * static_cast<int8_t> (tan.z());
			tPtr[3] = handedness;
		}
	}
}

// -----------------------------------------------------------------------------

inline bool isZero(float f, float tolerance=std::numeric_limits<float>::epsilon()) {
	return std::abs(f) <= tolerance;
}

// -----------------------------------------------------------------------------

//!	(static)
void cutMesh(Mesh* m, const Geometry::Plane& plane, const std::set<uint32_t> tIndices, float tolerance) {
	const VertexDescription & vd = m->getVertexDescription();
	const VertexAttribute & posAttr = vd.getAttribute(VertexAttributeIds::POSITION);
	if (posAttr.getDataType() != Util::TypeConstant::FLOAT || m->getDrawMode() != Mesh::DRAW_TRIANGLES) {
		WARN("cutMesh: Unsupported vertex format.");
		return;
	}

	std::deque<SplitTriangle> triangles;
	std::deque<SplitTriangle> trianglesOut;
	std::deque<SplitTriangle> trianglesNew;
	std::vector<RawVertex> vertexArray;

	MeshVertexData & vertices = m->openVertexData();
	MeshIndexData & indices = m->openIndexData();

	// extract triangles
	size_t vertexSize = vd.getVertexSize();
	for (uint32_t i = 0; i < vertices.getVertexCount(); ++i) {
		auto tmpData = new uint8_t[vertexSize];
		std::copy(vertices[i], vertices[i] + vertexSize, tmpData);
		vertexArray.emplace_back(i, tmpData, vertexSize);
	}
	uint32_t * iData = indices.data();
	for (unsigned i = 0; i < indices.getIndexCount(); i += 3)
		triangles.push_back(SplitTriangle(vertexArray.at(iData[i + 0]), vertexArray.at(iData[i + 1]), vertexArray.at(iData[i + 2])));

	// split triangles intersecting plane
	uint32_t tIndex = 0;
	while(!triangles.empty()) {
		auto t = triangles.front();
		triangles.pop_front();
		RawVertex a = t.getRawVertex(0);
		RawVertex b = t.getRawVertex(1);
		RawVertex c = t.getRawVertex(2);

		const Geometry::Vec3 va(reinterpret_cast<const float *> (a.getData() + posAttr.getOffset()));
		const Geometry::Vec3 vb(reinterpret_cast<const float *> (b.getData() + posAttr.getOffset()));
		const Geometry::Vec3 vc(reinterpret_cast<const float *> (c.getData() + posAttr.getOffset()));

		float pa = plane.planeTest(va);
		float pb = plane.planeTest(vb);
		float pc = plane.planeTest(vc);

		if( (pa>=-tolerance && pb>=-tolerance && pc>=-tolerance) || (pa<=tolerance && pb<=tolerance && pc<=tolerance) || (!tIndices.empty() && tIndices.count(tIndex)==0)) {
			// triangle is completely above/below plane -> keep
			trianglesOut.push_back(t);
		} else if (isZero(pa, tolerance) || isZero(pb, tolerance) || isZero(pc, tolerance)){
			// one point lies on the plane -> split into two triangles

			// reorder vertices s.t. vertex a lies on the plane
			if(isZero(pb, tolerance)) {
				std::swap(a, b); std::swap(b, c);
				std::swap(pa, pb); std::swap(pb, pc);
			} else if(isZero(pc, tolerance)) {
				std::swap(a, c); std::swap(b, c);
				std::swap(pa, pc); std::swap(pb, pc);
			}

			float blend = std::abs(pb)/(std::abs(pb) + std::abs(pc));
			RawVertex d = RawVertex::interpolate(b, c, blend, vertexArray.size(), vd);
			vertexArray.push_back(d);
			trianglesOut.push_back(SplitTriangle(a, b, d));
			trianglesNew.push_back(SplitTriangle(a, d, c));
		} else {
			// only one point is above/below plane -> split into three triangles

			// reorder vertices s.t. vertex a is the single point above/below the plane
			if( (pb>=0 && pa<=0 && pc<=0) || (pb<=0 && pa>=0 && pc>=0) ) { // b above/below plane
				std::swap(a, b); std::swap(b, c);
				std::swap(pa, pb); std::swap(pb, pc);
			} else if((pc>=0 && pa<=0 && pb<=0) || (pc<=0 && pa>=0 && pb>=0)) { // c above/below plane
				std::swap(a, c); std::swap(b, c);
				std::swap(pa, pc); std::swap(pb, pc);
			}

			float blend_ab = std::abs(pa)/(std::abs(pa) + std::abs(pb));
			float blend_ac = std::abs(pa)/(std::abs(pa) + std::abs(pc));
			RawVertex d_ab = RawVertex::interpolate(a, b, blend_ab, vertexArray.size(), vd);
			vertexArray.push_back(d_ab);
			RawVertex d_ac = RawVertex::interpolate(a, c, blend_ac, vertexArray.size(), vd);
			vertexArray.push_back(d_ac);

			trianglesOut.push_back(SplitTriangle(a, d_ab, d_ac));
			trianglesNew.push_back(SplitTriangle(d_ab, b, c));
			trianglesNew.push_back(SplitTriangle(d_ab, c, d_ac));
		}
		++tIndex;
	}
	for(auto t : trianglesNew)
		trianglesOut.push_back(t);
	trianglesNew.clear();

	// reassemble mesh
	// - indices
	uint32_t iCount = trianglesOut.size() * 3;
	indices.allocate(iCount);
	for (uint32_t i = 0; i < iCount; i += 3) {
		SplitTriangle t = trianglesOut.front();
		trianglesOut.pop_front();
		indices[i + 0] = t.a.getIndex();
		indices[i + 1] = t.b.getIndex();
		indices[i + 2] = t.c.getIndex();
	}
	indices.updateIndexRange();

	// - vertices
	vertices.allocate(vertexArray.size(), vd);
	for (size_t i = 0; i < vertexArray.size(); i++) {
		std::copy(vertexArray.at(i).getData(), vertexArray.at(i).getData() + vertexSize, vertices[i]);
	}
	vertices.updateBoundingBox();

	// cleanup
	for (auto & rawVertex : vertexArray) {
		delete[] rawVertex.getData();
	}
}

// -----------------------------------------------------------------------------

#define ADJ_AB 1
#define ADJ_BC 2
#define ADJ_CA 4

inline
uint8_t getAdjacence(const SplitTriangle& t1, const SplitTriangle& t2, const VertexAttribute & posAttr) {
	const static float EPS = std::numeric_limits<float>::epsilon()*10;
	const Geometry::Vec3 va1(reinterpret_cast<const float *> (t1.a.getData() + posAttr.getOffset()));
	const Geometry::Vec3 vb1(reinterpret_cast<const float *> (t1.b.getData() + posAttr.getOffset()));
	const Geometry::Vec3 vc1(reinterpret_cast<const float *> (t1.c.getData() + posAttr.getOffset()));
	const Geometry::Vec3 va2(reinterpret_cast<const float *> (t2.a.getData() + posAttr.getOffset()));
	const Geometry::Vec3 vb2(reinterpret_cast<const float *> (t2.b.getData() + posAttr.getOffset()));
	const Geometry::Vec3 vc2(reinterpret_cast<const float *> (t2.c.getData() + posAttr.getOffset()));

	bool eq_a = va1.equals(va2, EPS) || va1.equals(vb2, EPS) || va1.equals(vc2, EPS);
	bool eq_b = vb1.equals(va2, EPS) || vb1.equals(vb2, EPS) || vb1.equals(vc2, EPS);
	bool eq_c = vc1.equals(va2, EPS) || vc1.equals(vb2, EPS) || vc1.equals(vc2, EPS);

	//std::cout << eq_a << "; " << eq_b << "; " << eq_c << std::endl;
	//std::cout << va1 << "; " << vb1 << "; " << vc1 << std::endl;
	//std::cout << va2 << "; " << vb2 << "; " << vc2 << std::endl;

	if(eq_a && eq_b)
		return ADJ_AB;
	if(eq_b && eq_c)
		return ADJ_BC;
	if(eq_c && eq_a)
		return ADJ_CA;
	return 0;
}

// -----------------------------------------------------------------------------

//!	(static)
void extrudeTriangles(Mesh* m, const Geometry::Vec3& dir, const std::set<uint32_t> tIndices) {
	const VertexDescription & vd = m->getVertexDescription();
	const VertexAttribute & posAttr = vd.getAttribute(VertexAttributeIds::POSITION);
	if (posAttr.getDataType() != Util::TypeConstant::FLOAT || m->getDrawMode() != Mesh::DRAW_TRIANGLES) {
		WARN("extrudeTriangles: Unsupported vertex format.");
		return;
	}

	std::vector<SplitTriangle> triangles;
	std::vector<RawVertex> vertexArray;

	MeshVertexData & vertices = m->openVertexData();
	MeshIndexData & indices = m->openIndexData();

	// extract triangles
	size_t vertexSize = vd.getVertexSize();
	for (uint32_t i = 0; i < vertices.getVertexCount(); ++i) {
		auto tmpData = new uint8_t[vertexSize];
		std::copy(vertices[i], vertices[i] + vertexSize, tmpData);
		vertexArray.emplace_back(i, tmpData, vertexSize);
	}
	uint32_t * iData = indices.data();
	for (unsigned i = 0; i < indices.getIndexCount(); i += 3)
		triangles.push_back(SplitTriangle(vertexArray.at(iData[i + 0]), vertexArray.at(iData[i + 1]), vertexArray.at(iData[i + 2])));


	// find adjacent triangles
	std::unordered_map<uint32_t, uint8_t> adjacencies;
	for(auto ti : tIndices) {
		if(ti >= triangles.size())
			continue;
		adjacencies[ti] = 0;
		for(auto tj : tIndices) {
			if(tj >= triangles.size() || ti == tj)
				continue;
			uint8_t adj = getAdjacence(triangles[ti], triangles[tj], posAttr);
			adjacencies[ti] |= adj;
			//std::cout << ti << "-" << tj << " ab " << ((adj&ADJ_AB)>0) << " bc " << ((adj&ADJ_BC)>0) << " ca " << ((adj&ADJ_CA)>0) << std::endl;
		}
	}

	// extrude triangles
	for(auto ti : tIndices) {
		if(ti >= triangles.size())
			continue;
		RawVertex a = triangles[ti].a;
		RawVertex b = triangles[ti].b;
		RawVertex c = triangles[ti].c;

		RawVertex an = RawVertex::move(a, dir, vertexArray.size(), vd);
		vertexArray.push_back(an);
		RawVertex bn = RawVertex::move(b, dir, vertexArray.size(), vd);
		vertexArray.push_back(bn);
		RawVertex cn = RawVertex::move(c, dir, vertexArray.size(), vd);
		vertexArray.push_back(cn);
		triangles[ti].a = an;
		triangles[ti].b = bn;
		triangles[ti].c = cn;

		// add new triangles
		uint8_t adj = adjacencies[ti];

		if( (adj & ADJ_CA) == 0) {
			triangles.push_back(SplitTriangle(a, an, cn));
			triangles.push_back(SplitTriangle(a, cn, c));
		}

		if( (adj & ADJ_AB) == 0) {
			triangles.push_back(SplitTriangle(b, bn, an));
			triangles.push_back(SplitTriangle(b, an, a));
		}

		if( (adj & ADJ_BC) == 0) {
			triangles.push_back(SplitTriangle(c, cn, bn));
			triangles.push_back(SplitTriangle(c, bn, b));
		}
	}

	// reassemble mesh
	// - indices
	{
		indices.allocate(triangles.size() * 3);
		uint32_t i=0;
		for (auto t : triangles) {
			indices[i++] = t.a.getIndex();
			indices[i++] = t.b.getIndex();
			indices[i++] = t.c.getIndex();
		}
		indices.updateIndexRange();
	}

	// - vertices
	vertices.allocate(vertexArray.size(), vd);
	for (size_t i = 0; i < vertexArray.size(); i++) {
		std::copy(vertexArray.at(i).getData(), vertexArray.at(i).getData() + vertexSize, vertices[i]);
	}
	vertices.updateBoundingBox();

	// cleanup
	for (auto & rawVertex : vertexArray) {
		delete[] rawVertex.getData();
	}
}

//!	(static)
int32_t getFirstTriangleIntersectingRay(Mesh* m, const Geometry::Ray3& ray) {
	if (m->getDrawMode() != Mesh::DRAW_TRIANGLES) {
		WARN("getFirstTriangleIntersectingRay: Unsupported vertex format.");
		return -1;
	}
	auto posAcc = PositionAttributeAccessor::create(m->openVertexData(), VertexAttributeIds::POSITION);
	MeshIndexData & indices = m->openIndexData();

	float tLine, uTri, vTri;
	int32_t closest = -1;
	float closestDist = std::numeric_limits<float>::infinity();
	for(uint32_t i=0; i<indices.getIndexCount(); i+=3) {
		Geometry::Triangle<Geometry::Vec3> triangle(
			posAcc->getPosition(indices[i+0]),
			posAcc->getPosition(indices[i+1]),
			posAcc->getPosition(indices[i+2]));
		// TODO: check triangle normal
		if(Geometry::Intersection::getLineTriangleIntersection(ray, triangle, tLine, uTri, vTri)) {
			if(tLine >= 0 && tLine < closestDist) {
				closestDist = tLine;
				closest = i/3;
			}
		}
	}

	return closest;
}

// -----------------------------------------------------------------------------


//! (static)
uint32_t mergeCloseVertices(Mesh * mesh, float tolerance) {
	const VertexDescription & desc = mesh->getVertexDescription();
	const uint32_t indexCount = mesh->getIndexCount();

	auto posAcc = PositionAttributeAccessor::create(mesh->openVertexData(), VertexAttributeIds::POSITION);

	auto comp = [&posAcc, &tolerance](const RawVertex& lhs, const RawVertex& rhs) -> bool {
		auto v1 = posAcc->getPosition(lhs.getIndex());
		auto v2 = posAcc->getPosition(rhs.getIndex());
		if(std::abs(v1.x()-v2.x()) > tolerance)
			return v1.x() < v2.x();
		if(std::abs(v1.y()-v2.y()) > tolerance)
			return v1.y() < v2.y();
		if(std::abs(v1.z()-v2.z()) > tolerance)
			return v1.z() < v2.z();
		return false;
	};

	uint32_t oldCount = mesh->getVertexCount();

	// Set of vertices used to create unique vertices.
	std::set<RawVertex, decltype(comp)> rawVertices(comp);
	// Mapping from old index to new index.
	std::map<uint32_t, uint32_t> indexReplace;

	// Simply go over the indices and add one vertex after another.
	{
		const MeshVertexData & vertices = mesh->openVertexData();
		const MeshIndexData & indices = mesh->openIndexData();
		const std::size_t vertexSize = desc.getVertexSize();

		for (uint32_t counter = 0; counter < indexCount; ++counter) {
			const uint32_t index = indices[counter];
			const uint8_t * vertex = vertices[index];
			RawVertex raw(index, vertex, vertexSize);
			auto it = rawVertices.insert(raw);
			indexReplace.insert(std::make_pair(index, it.first->getIndex()));
		}
	}

	// Mapping from new index to vertex position.
	std::map<uint32_t, uint32_t> indexPosition;

	// Create the new mesh and add the rawVertices.
	Util::Reference<Mesh> result = new Mesh;
	result->setDataStrategy(mesh->getDataStrategy());
	result->setFileName(mesh->getFileName());
	result->setUseIndexData(mesh->isUsingIndexData());
	result->setDrawMode(mesh->getDrawMode());

	MeshVertexData & vertices = result->openVertexData();
	vertices.allocate(rawVertices.size(), desc);

	MeshIndexData & indices = result->openIndexData();
	indices.allocate(indexCount);

	{
		uint8_t * data = vertices.data();
		uint32_t vertexPos = 0;
		for(const auto & vertex : rawVertices) {
			std::copy(vertex.getData(), vertex.getData() + vertex.getSize(), data);
			data += vertex.getSize();
			indexPosition.insert(std::make_pair(vertex.getIndex(), vertexPos));
			++vertexPos;
		}
		// Translate the indices.
		const uint32_t * srcIndex = mesh->openIndexData().data();
		uint32_t * dstIndex = indices.data();
		for (uint32_t counter = 0; counter < indexCount; ++counter) {
			const uint32_t newIndex = indexReplace.find(*srcIndex)->second;
			*dstIndex = indexPosition.find(newIndex)->second;
			++srcIndex;
			++dstIndex;
		}
	}

	vertices.updateBoundingBox();
	indices.updateIndexRange();

	mesh->swap(*result.get());

	return oldCount-mesh->getVertexCount();
}

// -----------------------------------------------------------------------------

std::deque<Mesh*> splitIntoConnectedComponents(Mesh* mesh, float relDistance/*=0.001*/) {
	std::deque<Mesh*> result;
	auto bb = mesh->getBoundingBox();
	float distance =bb.getDiameter() * relDistance;
	
	if(mesh->getDrawMode() != Mesh::DRAW_TRIANGLES) {
		WARN("Mesh is not a triangle mesh.");
		return result;
	}
	
	struct ConnectedComponent;
	struct Triangle {
		uint32_t idx;
		ConnectedComponent* component;
		Triangle(uint32_t i) : idx(i), component(nullptr) {}
	};	
	struct ConnectedComponent {
		std::vector<Triangle*> triangles;
		void join(ConnectedComponent* other) {
			if(other != this) {
				if(other->triangles.size() < triangles.size()) {
					for(auto& t : other->triangles) 
						t->component = this;
					triangles.reserve(triangles.size() + other->triangles.size());
					triangles.insert(triangles.end(), other->triangles.begin(), other->triangles.end());
					other->triangles.clear();
				} else {
					for(auto& t : triangles) 
						t->component = other;
					other->triangles.reserve(triangles.size() + other->triangles.size());
					other->triangles.insert(other->triangles.end(), triangles.begin(), triangles.end());
					triangles.clear();
				}
			}
		}
		void add(Triangle* t) {
			t->component = this;
			triangles.emplace_back(t);
		}
	};
	struct OctreeEntry : public Geometry::Point<Geometry::Vec3f> {
		Triangle* triangle;
		OctreeEntry(const Geometry::Vec3 & p, Triangle* tri) : Geometry::Point<Geometry::Vec3f>(p), triangle(tri) {}
	};
	
	Geometry::PointOctree<OctreeEntry> octree(bb,distance,100);
	Geometry::Sphere_f searchSphere({0,0,0}, distance);
	std::vector<std::unique_ptr<ConnectedComponent>> components;
	std::vector<std::unique_ptr<Triangle>> triangles;
	triangles.reserve(mesh->getPrimitiveCount());
	
	auto triAcc = TriangleAccessor::create(mesh);
	std::cout << "Identifying connected components " << 0 << "%        ";
	for(uint32_t i=0; i<mesh->getPrimitiveCount(); ++i) {
		auto triangle = new Triangle(i);
		triangles.emplace_back(triangle);
		auto tri = triAcc->getTriangle(i);
		for(const auto& pos : {tri.getVertexA(), tri.getVertexB(), tri.getVertexC()}) {
			searchSphere.setCenter(pos);
			std::deque<OctreeEntry> points;
			octree.collectPointsWithinSphere(searchSphere, points);
			for(auto& point : points) {
				auto otherTriangle = point.triangle;
				if(!triangle->component)
					otherTriangle->component->add(triangle);
				else
					otherTriangle->component->join(triangle->component);
			}
		}
		if(!triangle->component) {
			auto cc = new ConnectedComponent;
			cc->add(triangle);
			components.emplace_back(cc);
		}		
		for(const auto& pos : {tri.getVertexA(), tri.getVertexB(), tri.getVertexC()}) {
			octree.insert({pos, triangle});
		}
		if(i%1000==0) {
			std::cout << "\rIdentifying connected components " << (static_cast<float>(i*100)/static_cast<float>(mesh->getPrimitiveCount())) << "%        ";
		}
	}
	std::cout << "\rIdentifying connected components 100%        " << std::endl;
	
	uint32_t cmpCount=0;
	for(const auto& cc : components) {
		if(cc->triangles.size() > 0) {
			++cmpCount;
		}
	}
	
	std::cout << "Identifyied " << cmpCount << " components" << std::endl;
	std::cout << "Creating meshes 0%        ";
	uint32_t j=0;
	
	MeshIndexData tmpIndexData;
	tmpIndexData.allocate(1);
	MeshVertexData vertexData(mesh->openVertexData());
	Mesh tmpMesh(tmpIndexData, vertexData);
	tmpMesh.setDataStrategy(mesh->getDataStrategy());
	tmpMesh.setDrawMode(mesh->getDrawMode());
	tmpMesh.setUseIndexData(true);
	
	for(const auto& cc : components) {
		if(cc->triangles.size() > 0) {
			MeshIndexData indexData;
			indexData.allocate(cc->triangles.size()*3);
			uint32_t i=0;
			for(auto t : cc->triangles) {
				auto triIdx = triAcc->getIndices(t->idx);
				indexData[i++] = std::get<0>(triIdx);
				indexData[i++] = std::get<1>(triIdx);
				indexData[i++] = std::get<2>(triIdx);
			}
			tmpMesh.openIndexData().swap(indexData);
			result.push_back(eliminateUnusedVertices(&tmpMesh));
			std::cout << "\rCreating meshes " << (static_cast<float>(++j*100)/static_cast<float>(cmpCount)) << "%        ";
		}
	}
	std::cout << "\rCreating meshes 100%        " << std::endl; 
	
	return result;
}

// -----------------------------------------------------------------------------

void applyDisplacementMap(Mesh* mesh, Util::PixelAccessor* displaceAcc, float scale, bool clampToEdge) {
	if(!mesh || !displaceAcc) {
		return;
	} 
	const auto& vd = mesh->getVertexDescription();
	if(!vd.hasAttribute(VertexAttributeIds::NORMAL) || !vd.hasAttribute(VertexAttributeIds::TEXCOORD0)) {
		WARN("applyDisplacementMap: Mesh requires normals and texture coordinates.");
		return;
	}
	const uint32_t width = displaceAcc->getWidth();
	const uint32_t height = displaceAcc->getHeight();
	auto& vData = mesh->openVertexData();
	auto pAcc(PositionAttributeAccessor::create(vData, VertexAttributeIds::POSITION));
	auto tcAcc(TexCoordAttributeAccessor::create(vData, VertexAttributeIds::TEXCOORD0));
	auto nAcc(NormalAttributeAccessor::create(vData, VertexAttributeIds::NORMAL));
	for(uint32_t i=0; i<mesh->getVertexCount(); ++i) {
		auto pos = pAcc->getPosition(i);
		auto tc = tcAcc->getCoordinate(i);
		auto n = nAcc->getNormal(i);
		uint32_t px = clampToEdge ? std::max(0, std::min<int32_t>(width-1, tc.x()*width)) : ((tc.x() - std::floor(tc.x())) * width);
		uint32_t py = clampToEdge ? std::max(0, std::min<int32_t>(height-1, tc.y()*height)) : ((tc.y() - std::floor(tc.y())) * height);
		auto value = displaceAcc->readSingleValueFloat(px, py) * scale;
		pAcc->setPosition(i, pos + n * value);
	}
	vData.markAsChanged();
}

// -----------------------------------------------------------------------------

void applyNoise(Mesh* mesh, float noiseScale, const Geometry::Matrix4x4& transform, uint32_t seed) {
	if(!mesh)
		return;
	
	const auto& vd = mesh->getVertexDescription();
	if(!vd.hasAttribute(VertexAttributeIds::NORMAL)) {
		WARN("applyNoise: Mesh requires normals.");
		return;
	}
	
	Util::NoiseGenerator gen(seed);
	
	auto& vData = mesh->openVertexData();
	auto pAcc(PositionAttributeAccessor::create(vData, VertexAttributeIds::POSITION));
	auto nAcc(NormalAttributeAccessor::create(vData, VertexAttributeIds::NORMAL));
	for(uint32_t i=0; i<mesh->getVertexCount(); ++i) {
		auto pos = pAcc->getPosition(i);
		auto tPos = transform.transformPosition(pos);
		auto n = nAcc->getNormal(i);
		auto value = gen.get(tPos.x(), tPos.y(), tPos.z()) * noiseScale;
		pAcc->setPosition(i, pos + n * value);
	}
	vData.markAsChanged();
	vData.updateBoundingBox();
}

// -----------------------------------------------------------------------------

void flattenMesh(Mesh* mesh, const Geometry::Vec3& pos, float radius, float falloff) {
	using namespace Geometry;
	if(!mesh) return;
	auto& vData = mesh->openVertexData();
	auto pAcc(PositionAttributeAccessor::create(vData, VertexAttributeIds::POSITION));
	for(uint32_t i=0; i<mesh->getVertexCount(); ++i) {
		auto p = pAcc->getPosition(i);
		float d = pos.distance(p);
		if(d <= radius) {
			p.y(pos.y());
		} else if(d < radius+falloff) {
			float b = (d-radius)/falloff;
			p.y(Interpolation::cubicBezier(pos.y(),pos.y(),p.y(), p.y(), b));
		}
		pAcc->setPosition(i, p);
	}
}

// -----------------------------------------------------------------------------

float computeSurfaceArea(Mesh* mesh) {
	if(!mesh || mesh->getDrawMode() != Mesh::DRAW_TRIANGLES) return 0;
	auto tAcc = TriangleAccessor::create(mesh);
	float area = 0;
	for(uint32_t i=0; i<mesh->getPrimitiveCount(); ++i) 
		area += tAcc->getTriangle(i).calcArea();
	return area;
}

// -----------------------------------------------------------------------------

MeshVertexData* extractVertices(Mesh* mesh, const std::vector<uint32_t>& indices) {
  const VertexDescription & desc = mesh->getVertexDescription();
  MeshVertexData & meshVertices = mesh->openVertexData();

  if(indices.empty())
    return nullptr;

  auto result = new MeshVertexData;
  result->allocate(indices.size(), desc);
  
  uint32_t i=0;
  for(const auto& index : indices) {
    uint32_t start = index * desc.getVertexSize();
    uint32_t end = start + desc.getVertexSize();
    std::copy(meshVertices.data() + start, meshVertices.data() + end, result->data() + desc.getVertexSize()*i++);
  }

  return result;
}

// -----------------------------------------------------------------------------

void copyVertices(Rendering::Mesh* source, Rendering::Mesh* target, uint32_t sourceOffset, uint32_t targetOffset, uint32_t count) {
	const auto& vd = source->getVertexDescription();
	if(!(target->getVertexDescription() == vd)) {
		WARN("copyVertices: Source and target mesh have incompatible vertex descriptions.");
		return;
	}	
	if(source->getVertexCount() < sourceOffset+count) {
		WARN("copyVertices: Not enough source vertices available.");
		return;
	}	
	if(target->getVertexCount() < targetOffset+count) {
		WARN("copyVertices: Target vertex count is too small.");
		return;
	}
	
	auto& srcVertices = source->_getVertexData();
	auto& tgtVertices = target->_getVertexData();
	
	uint32_t srcStart = sourceOffset*vd.getVertexSize();
	uint32_t tgtStart = targetOffset*vd.getVertexSize();
	uint32_t srcEnd = (sourceOffset + count)*vd.getVertexSize();
		
	if(srcVertices.isUploaded() && tgtVertices.isUploaded()) {
		BufferObject srcBO;
		BufferObject tgtBO;
		srcVertices._swapBufferObject(srcBO);
		tgtVertices._swapBufferObject(tgtBO);
		tgtBO.copy(srcBO, srcStart, tgtStart, count*vd.getVertexSize());
		srcVertices._swapBufferObject(srcBO);
		tgtVertices._swapBufferObject(tgtBO);
		tgtVertices.releaseLocalData();
	} else if(srcVertices.hasLocalData() && !tgtVertices.hasLocalData()) {
		tgtVertices.download();
	} else if(!srcVertices.hasLocalData() && tgtVertices.hasLocalData()) {
		srcVertices.download();
	}
	
	if(srcVertices.hasLocalData() && tgtVertices.hasLocalData()) {
		std::copy(srcVertices.data() + srcStart, srcVertices.data() + srcEnd, tgtVertices.data() + tgtStart);
		tgtVertices.markAsChanged();
	}
	
}

// -----------------------------------------------------------------------------

}
}

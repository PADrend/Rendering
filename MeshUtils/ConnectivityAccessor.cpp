/*
 This file is part of the Rendering library.
 Copyright (C) 2015 Sascha Brandt <myeti@mail.upb.de>

 This library is subject to the terms of the Mozilla Public License, v. 2.0.
 You should have received a copy of the MPL along with this library; see the
 file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ConnectivityAccessor.h"

#include "../Mesh/Mesh.h"
#include "../Mesh/MeshIndexData.h"
#include "../Mesh/MeshVertexData.h"
#include "../Mesh/VertexAttributeAccessors.h"
#include "../Mesh/VertexAttributeIds.h"

#include <Util/StringUtils.h>

#include <Geometry/Vec3.h>
#include <Geometry/Triangle.h>

#include <limits>

#define INVALID std::numeric_limits<uint32_t>::max()

namespace Rendering {
namespace MeshUtils {

static const std::string unimplementedFormatMsg("Mesh is not a valid triangle mesh.");

void ConnectivityAccessor::assertCornerRange(uint32_t cIndex) const {
	if(cIndex >= indices.getIndexCount())
		throw std::invalid_argument("Trying to access corner " + Util::StringUtils::toString(cIndex) + " of overall " + Util::StringUtils::toString(indices.getIndexCount()) + " corners.");
}

void ConnectivityAccessor::assertVertexRange(uint32_t vIndex) const {
	if(vIndex >= vertexCorners.size())
		throw std::invalid_argument("Trying to access vertex " + Util::StringUtils::toString(vIndex) + " of overall " + Util::StringUtils::toString(vertexCorners.size()) + " vertices.");
}

void ConnectivityAccessor::assertTriangleRange(uint32_t tIndex) const {
	if(tIndex*3 >= indices.getIndexCount())
		throw std::invalid_argument("Trying to access triangle " + Util::StringUtils::toString(tIndex) + " of overall " + Util::StringUtils::toString(indices.getIndexCount()/3) + " triangles.");
}

ConnectivityAccessor::ConnectivityAccessor(Mesh* mesh) : indices(mesh->openIndexData()),
		posAcc(PositionAttributeAccessor::create(mesh->openVertexData(), VertexAttributeIds::POSITION)),
		triAcc(TriangleAccessor::create(mesh)) {
	vertexCorners.resize(mesh->getVertexCount(), INVALID);
	triangleNextCorners.resize(indices.getIndexCount(), INVALID);


	// TODO: do this lazy, on demand, and only partially.
	for(uint32_t i=0; i<indices.getIndexCount(); ++i) {
		uint32_t c = vertexCorners[indices[i]];
		if(c == INVALID) {
			vertexCorners[indices[i]] = i;
			triangleNextCorners[i] = i;
		} else {
			uint32_t j = c;
			uint32_t nvc = triangleNextCorners[j];
			while(nvc != INVALID && nvc != c) {
				j = nvc;
				nvc = triangleNextCorners[j];
			}
			triangleNextCorners[j] = i;
			triangleNextCorners[i] = c;
		}
	}
}

//! (static)
Util::Reference<ConnectivityAccessor> ConnectivityAccessor::create(Mesh* mesh) {
	if(mesh->isUsingIndexData() && mesh->getDrawMode() == Mesh::DRAW_TRIANGLES) {
		return new ConnectivityAccessor(mesh);
	} else {
		throw std::invalid_argument(unimplementedFormatMsg + '\'');
	}
}

Geometry::Vec3 ConnectivityAccessor::getVertex(uint32_t vIndex) const {
	assertVertexRange(vIndex);
	return posAcc->getPosition(vIndex);
}

TriangleAccessor::TriangleIndices_t ConnectivityAccessor::getTriangle(uint32_t tIndex) const {
	assertTriangleRange(tIndex);
	return triAcc->getIndices(tIndex);
}

uint32_t ConnectivityAccessor::getCorner(uint32_t vIndex, uint32_t tIndex) const {
	assertVertexRange(vIndex);
	assertTriangleRange(tIndex);
	uint32_t c = vertexCorners[vIndex];
	while(c != INVALID && c/3 != tIndex)
		c = triangleNextCorners[c];
	return c;
}

uint32_t ConnectivityAccessor::getVertexCorner(uint32_t vIndex) const {
	assertVertexRange(vIndex);
	return vertexCorners[vIndex];
}

uint32_t ConnectivityAccessor::getTriangleCorner(uint32_t tIndex) const {
	assertTriangleRange(tIndex);
	return tIndex*3;
}

uint32_t ConnectivityAccessor::getCornerVertex(uint32_t cIndex) const {
	assertCornerRange(cIndex);
	return indices[cIndex];
}

uint32_t ConnectivityAccessor::getCornerTriangle(uint32_t cIndex) const {
	assertCornerRange(cIndex);
	return cIndex/3;
}

uint32_t ConnectivityAccessor::getNextVertexCorner(uint32_t cIndex) const {
	assertCornerRange(cIndex);
	return triangleNextCorners[cIndex];
}

uint32_t ConnectivityAccessor::getNextTriangleCorner(uint32_t cIndex) const {
	assertCornerRange(cIndex);
	uint32_t tIndex = (cIndex/3);
	return tIndex*3+((cIndex+1)%3);
}

std::vector<uint32_t> ConnectivityAccessor::getVertexAdjacentTriangles(uint32_t vIndex) const {
	std::vector<uint32_t> out;
	uint32_t c = getVertexCorner(vIndex);
	out.push_back(getCornerTriangle(c));
	uint32_t nc = getNextVertexCorner(c);
	while(nc != c && nc != INVALID) {
		out.push_back(getCornerTriangle(nc));
		nc = getNextVertexCorner(nc);
	}
	return out;
}

std::vector<uint32_t> ConnectivityAccessor::getAdjacentTriangles(uint32_t tIndex) const {
	auto triangle = getTriangle(tIndex);
	std::vector<uint32_t> out;
	auto e1 = std::make_pair(std::get<0>(triangle), std::get<1>(triangle)); // edge a-b
	auto e2 = std::make_pair(std::get<1>(triangle), std::get<2>(triangle)); // edge b-c
	auto e3 = std::make_pair(std::get<2>(triangle), std::get<0>(triangle)); // edge c-a
	for(auto e : {e1,e2,e3}) {
		for(auto t : getVertexAdjacentTriangles(e.first)) {
			uint32_t c = getTriangleCorner(t);
			while(getCornerVertex(c) != e.first)
				c = getNextTriangleCorner(c);
			uint32_t pc = getNextTriangleCorner(getNextTriangleCorner(c)); // previous corner = 2 * next corner
			if(e.second == getCornerVertex(pc))
				out.push_back(t);
		}
	}
	return out;
}

bool ConnectivityAccessor::isBorderEdge(uint32_t vIndex1, uint32_t vIndex2) const {
	// find an edge with vertex1 and vertex2
	uint32_t c = getVertexCorner(vIndex1);
	uint32_t ntc = getNextTriangleCorner(c);
	uint32_t nvc = getNextVertexCorner(c);
	while(getCornerVertex(ntc) != vIndex2 && nvc != c) {
		ntc = getNextTriangleCorner(nvc);
		nvc = getNextVertexCorner(nvc);
	}
	if(getCornerVertex(ntc) != vIndex2 )
		return false; // not an edge

	ntc = getNextTriangleCorner(getNextTriangleCorner(c));
	nvc = getNextVertexCorner(c);
	while(nvc != c) {
		if(getCornerVertex(ntc) == vIndex2)
			return false; // opposing edge found
		ntc = getNextTriangleCorner(getNextTriangleCorner(nvc));
		nvc = getNextVertexCorner(nvc);
	}
	return getCornerVertex(ntc) != vIndex2;
}

bool ConnectivityAccessor::isBorderTriangle(uint32_t tIndex) const {
	auto tri = getTriangle(tIndex);
	return isBorderEdge(std::get<0>(tri), std::get<1>(tri)) ||
		   isBorderEdge(std::get<1>(tri), std::get<2>(tri)) ||
		   isBorderEdge(std::get<2>(tri), std::get<0>(tri));
}

} /* namespace MeshUtils */
} /* namespace Rendering */

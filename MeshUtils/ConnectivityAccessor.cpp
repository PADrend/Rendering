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

#include <Geometry/Vec3.h>
#include <Geometry/Triangle.h>

#include <limits>

namespace Rendering {
namespace MeshUtils {

static const std::string unimplementedFormatMsg("Mesh is not a valid triangle mesh.");

void ConnectivityAccessor::assertCornerRange(uint32_t cIndex) const {
	if(cIndex >= indices.getIndexCount())
		throw std::invalid_argument("Trying to access corner " + std::to_string(cIndex) + " of overall " + std::to_string(indices.getIndexCount()) + " corners.");
}

void ConnectivityAccessor::assertVertexRange(uint32_t vIndex) const {
	if(vIndex >= vertexCorners.size())
		throw std::invalid_argument("Trying to access vertex " + std::to_string(vIndex) + " of overall " + std::to_string(vertexCorners.size()) + " vertices.");
}

void ConnectivityAccessor::assertTriangleRange(uint32_t tIndex) const {
	if(tIndex*3 >= indices.getIndexCount())
		throw std::invalid_argument("Trying to access triangle " + std::to_string(tIndex) + " of overall " + std::to_string(indices.getIndexCount()/3) + " triangles.");
}

ConnectivityAccessor::ConnectivityAccessor(Mesh* mesh) : indices(mesh->openIndexData()),
		posAcc(PositionAttributeAccessor::create(mesh->openVertexData(), VertexAttributeIds::POSITION)),
		triAcc(TriangleAccessor::create(mesh)) {
	vertexCorners.resize(mesh->getVertexCount(), std::numeric_limits<uint32_t>::max());
	triangleNextCorners.resize(indices.getIndexCount(), std::numeric_limits<uint32_t>::max());


	// TODO: do this lazy, on demand, and only partially.
	for(uint32_t i=0; i<indices.getIndexCount(); ++i) {
		uint32_t c = vertexCorners[indices[i]];
		if(c == std::numeric_limits<uint32_t>::max()) {
			vertexCorners[indices[i]] = i;
			triangleNextCorners[i] = i;
		} else {
			uint32_t j = c;
			uint32_t nvc = triangleNextCorners[j];
			while(nvc != std::numeric_limits<uint32_t>::max() && nvc != c) {
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
	while(c != std::numeric_limits<uint32_t>::max() && c/3 != tIndex)
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
	return tIndex+((cIndex+1)%3);
}


} /* namespace MeshUtils */
} /* namespace Rendering */

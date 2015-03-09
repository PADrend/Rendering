/*
 This file is part of the Rendering library.
 Copyright (C) 2015 Sascha Brandt <myeti@mail.upb.de>

 This library is subject to the terms of the Mozilla Public License, v. 2.0.
 You should have received a copy of the MPL along with this library; see the
 file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "TriangleAccessor.h"

#include "../Mesh/Mesh.h"
#include "../Mesh/MeshIndexData.h"
#include "../Mesh/MeshVertexData.h"
#include "../Mesh/VertexAttributeAccessors.h"
#include "../Mesh/VertexAttributeIds.h"

#include <Util/StringUtils.h>

#include <Geometry/Vec3.h>
#include <Geometry/Triangle.h>

namespace Rendering {
namespace MeshUtils {

static const std::string unimplementedFormatMsg("Mesh is not a valid triangle mesh.");

TriangleAccessor::TriangleAccessor(Mesh* mesh) : indices(mesh->openIndexData()),
		posAcc(PositionAttributeAccessor::create(mesh->openVertexData(), VertexAttributeIds::POSITION)),
		meshDataHolder(new LocalMeshDataHolder(mesh)) {
}


void TriangleAccessor::assertRange(uint32_t index) const {
	if(index*3 >= indices.getIndexCount())
		throw std::invalid_argument("Trying to access triangle " + Util::StringUtils::toString(index) + " of overall " + Util::StringUtils::toString(indices.getIndexCount()/3) + " triangles.");
}

//! (static)
Util::Reference<TriangleAccessor> TriangleAccessor::create(Mesh* mesh) {
	if(mesh->isUsingIndexData() && mesh->getDrawMode() == Mesh::DRAW_TRIANGLES) {
		return new TriangleAccessor(mesh);
	} else {
		throw std::invalid_argument(unimplementedFormatMsg + '\'');
	}
}

Geometry::Triangle3 TriangleAccessor::getTriangle(uint32_t index) const {
	assertRange(index);
	auto v1 = posAcc->getPosition(indices[index*3+0]);
	auto v2 = posAcc->getPosition(indices[index*3+1]);
	auto v3 = posAcc->getPosition(indices[index*3+2]);
	return Geometry::Triangle3(v1, v2, v3);
}

void TriangleAccessor::setTriangle(uint32_t index, const Geometry::Triangle3 triangle) {
	assertRange(index);
	posAcc->setPosition(indices[index*3+0], triangle.getVertexA());
	posAcc->setPosition(indices[index*3+1], triangle.getVertexB());
	posAcc->setPosition(indices[index*3+2], triangle.getVertexC());
}

TriangleAccessor::TriangleIndices_t TriangleAccessor::getIndices(uint32_t index) const {
	assertRange(index);
	return std::make_tuple(indices[index*3+0],indices[index*3+1],indices[index*3+2]);
}



} /* namespace MeshUtils */
} /* namespace Rendering */

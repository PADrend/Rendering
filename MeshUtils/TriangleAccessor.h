/*
 This file is part of the Rendering library.
 Copyright (C) 2015 Sascha Brandt <myeti@mail.upb.de>

 This library is subject to the terms of the Mozilla Public License, v. 2.0.
 You should have received a copy of the MPL along with this library; see the
 file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef RENDERING_TRIANGLEACCESSOR_H_
#define RENDERING_TRIANGLEACCESSOR_H_

#include "LocalMeshDataHolder.h"

#include <Util/References.h>
#include <Util/ReferenceCounter.h>

#include <tuple>
#include <memory>

namespace Geometry {
template<typename _T> class _Vec3;
typedef _Vec3<float> Vec3;
template<typename T_> class Triangle;
typedef Triangle<Vec3> Triangle3;
}

namespace Rendering {
class Mesh;
class MeshVertexData;
class MeshIndexData;
class PositionAttributeAccessor;
namespace MeshUtils {

/**
 * Allows to get triangles of a mesh.
 * @ingroup mesh_accessor
 */
class TriangleAccessor : public Util::ReferenceCounter<TriangleAccessor> {
private:
	MeshIndexData& indices;
	Util::Reference<PositionAttributeAccessor> posAcc;
	std::unique_ptr<LocalMeshDataHolder> meshDataHolder;
protected:
	RENDERINGAPI TriangleAccessor(Mesh* mesh);

	RENDERINGAPI void assertRange(uint32_t index) const;
public:
	typedef std::tuple<uint32_t,uint32_t,uint32_t> TriangleIndices_t;

	/*! (static factory)
		Create a TriangleAccessor for the given Mesh.
		If no Accessor can be created, an std::invalid_argument exception is thrown. */
	RENDERINGAPI static Util::Reference<TriangleAccessor> create(Mesh* mesh);

	virtual ~TriangleAccessor() {}

	RENDERINGAPI Geometry::Triangle3 getTriangle(uint32_t index) const;
	RENDERINGAPI void setTriangle(uint32_t index, const Geometry::Triangle3 triangle);

	RENDERINGAPI TriangleIndices_t getIndices(uint32_t index) const;
};

} /* namespace MeshUtils */
} /* namespace Rendering */

#endif /* RENDERING_TRIANGLEACCESSOR_H_ */

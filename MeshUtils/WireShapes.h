/*
  This file is part of the Rendering library.
  Copyright (C) 2019 Sascha Brandt <sascha@brandt.graphics>

  This library is subject to the terms of the Mozilla Public License, v. 2.0.
  You should have received a copy of the MPL along with this library; see the
  file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef RENDERING_MESHUTILS_WIRESHAPES_H_
#define RENDERING_MESHUTILS_WIRESHAPES_H_

#include <Util/References.h>

#include <cstdint>

namespace Geometry {
template<typename value_t> class _Box;
typedef _Box<float> Box;
template<typename value_t> class _Rect;
typedef _Rect<float> Rect;
template<typename value_t> class _Vec3;
typedef _Vec3<float> Vec3;
template<typename value_t> class _Sphere;
typedef _Sphere<float> Sphere;
class Frustum;
}

namespace Util {
class Color4f;
}

namespace Rendering {
class Mesh;
class VertexDescription;
namespace MeshUtils {
class MeshBuilder;

//! @ingroup mesh_builder
namespace WireShapes {

/**
 * Return the wireframe mesh of a three-dimensional, axis-aligned box.
 *
 * @param vd Vertex description specifying the vertex information to generate
 * @param box Geometric specification of the box
 * @return Mesh of the box
 */
Mesh* createWireBox(const VertexDescription& vd, const Geometry::Box& box);
  
//! Adds a wireframe box to the given meshBuilder. \see createWireBox(...)
void addWireBox(MeshBuilder& mb, const Geometry::Box& box);

/**
 * Returns a wireframe rectangle (oriented in x-y-plane)
 *
 * @param vd Vertex description specifying the vertex information to generate
 * @param rect the rectangle
 * @return Rectangle Mesh
 */
Mesh* createWireRectangle(const VertexDescription& vd, const Geometry::Rect& rect);

//! Adds an rectangle to the given meshBuilder. \see createWireRectangle(...)
void addWireRectangle(MeshBuilder& mb, const Geometry::Rect& rect);

/**
 * Return a wireframe sphere, consisting of 3 axis-aligned circles.
 * \see createWireCircle(...)
 *
 * @param vd Vertex description specifying the vertex information to generate
 * @param sphere the sphere
 * @param numSegments Number of segments for each circle
 * @return Sphere Mesh
 */
Mesh* createWireSphere(const VertexDescription& vd, const Geometry::Sphere& sphere, uint8_t numSegments);

//! Adds a shere to the given meshBuilder. \see createWireSphere(...)
void addWireSphere(MeshBuilder& mb, const Geometry::Sphere& sphere, uint8_t numSegments);

 /**
 * Return a wireframe circle (oriented in x-y-plane).
 *
 * @param vd Vertex description specifying the vertex information to generate
 * @param radius radius of the circle
 * @param numSegments Number of segments for the circle
 * @return Circle Mesh
 */
Mesh* createWireCircle(const VertexDescription& vd, float radius, uint8_t numSegments);

//! Adds a wireframe circle to the given meshBuilder. \see createWireCircle(...)
void addWireCircle(MeshBuilder& mb, float radius, uint8_t numSegments);

/**
 * Return a wireframe frustum.
 *
 * @param vd Vertex description specifying the vertex information to generate
 * @param frustum the frustum
 * @return frustum Mesh
 */
Mesh* createWireFrustum(const VertexDescription& vd, const Geometry::Frustum& frustum);

//! Adds a wireframe frustum to the given meshBuilder. \see createWireFrustum(...)
void addWireFrustum(MeshBuilder& mb, const Geometry::Frustum& frustum);

/**
 * Returns an line mesh.
 *
 * @param vd Vertex description specifying the vertex information to generate
 * @param start start of the line
 * @param end end of the line
 * @return Line Mesh
 */
Mesh* createLine(const VertexDescription& vd, const Geometry::Vec3& start, const Geometry::Vec3& end);

//! Adds a line to the given meshBuilder. \see createLine(...)
void addLine(MeshBuilder& mb, const Geometry::Vec3& start, const Geometry::Vec3& end);

} /* WireShapes */
} /* MeshUtils */
} /* Rendering */

#endif /* end of include guard: RENDERING_MESHUTILS_WIRESHAPES_H_ */

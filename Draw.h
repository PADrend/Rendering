/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef DRAW_H_
#define DRAW_H_

/**
 * @file
 * Draw functions for simple objects
 */

// Forward declarations
namespace Geometry {
template<typename value_t> class _Box;
typedef _Box<float> Box;
template<typename _T> class _Rect;
typedef _Rect<float> Rect;
template<typename _T> class _Vec3;
typedef _Vec3<float> Vec3f;
}
namespace Rendering {
class RenderingContext;
}
namespace Util {
class Color4ub;
}

/**
 * @brief %Rendering classes and functions
 *
 * Library containing classes and functions for 2D and 3D rendering.
 * This library serves as an abstraction layer for a low-level graphics library
 * (<a href="http://www.khronos.org/opengl">OpenGL</a>, <a href="http://www.khronos.org/opengles/">OpenGL ES 2.0</a>).
 */
namespace Rendering {

void drawAbsBox(RenderingContext & rc, const Geometry::Box & box);
void drawAbsBox(RenderingContext & rc, const Geometry::Box & box, const Util::Color4ub & color);
void drawAbsWireframeBox(RenderingContext & rc, const Geometry::Box & box);
void drawAbsWireframeBox(RenderingContext & rc, const Geometry::Box & box, const Util::Color4ub & color);
void drawBox(RenderingContext & rc, const Geometry::Box & box);
void drawBox(RenderingContext & rc, const Geometry::Box & box, const Util::Color4ub & color);
void drawWireframeBox(RenderingContext & rc, const Geometry::Box & box);
void drawWireframeBox(RenderingContext & rc, const Geometry::Box & box, const Util::Color4ub & color);

/**
 * @note Because OpenGL immediate mode is used inside this function,
 * the caller has to make sure that RenderingContext::applyChanges() is called before.
 */
void drawFastAbsBox(RenderingContext & rc, const Geometry::Box & box);


void drawFullScreenRect(RenderingContext & rc);
/**
 * Draw a quadrilateral in three-dimensional space.
 * The quadrilateral is given by four points.
 * Positions, normals and texture coordinates are generated for the vertices.
 */
void drawQuad(RenderingContext & rc, const Geometry::Vec3f & lowerLeft, const Geometry::Vec3f & lowerRight, const Geometry::Vec3f & upperRight,
				const Geometry::Vec3f & upperLeft);
/**
 * Set the current color and draw a quadrilateral in three-dimensional space.
 * @see drawQuad
 */
void drawQuad(RenderingContext & rc, const Geometry::Vec3f & lowerLeft, const Geometry::Vec3f & lowerRight, const Geometry::Vec3f & upperRight,
				const Geometry::Vec3f & upperLeft, const Util::Color4ub & color);
void drawWireframeRect(RenderingContext & rc, const Geometry::Rect & rect);
void drawWireframeRect(RenderingContext & rc, const Geometry::Rect & rect, const Util::Color4ub & color);

void drawRect(RenderingContext & rc, const Geometry::Rect & rect);
void drawRect(RenderingContext & rc, const Geometry::Rect & rect, const Util::Color4ub & color);

/**
 * Draw a triangle in three-dimensional space.
 * The triangle is given by three points.
 * Positions are generated for the vertices only.
 */
void drawTriangle(RenderingContext & rc, const Geometry::Vec3f & vertexA, const Geometry::Vec3f & vertexB, const Geometry::Vec3f & vertexC);

void drawVector(RenderingContext & rc, const Geometry::Vec3f & from, const Geometry::Vec3f & to);
void drawVector(RenderingContext & rc, const Geometry::Vec3f & from, const Geometry::Vec3f & to, const Util::Color4ub & color);

/**
 * Set the projection and modelview matrices to enable drawing in screen space.
 * 
 * @note The state before the call is saved. Call disable2DMode() to restore it.
 */
void enable2DMode(RenderingContext & rc);

//! Reset the projection and modelview matrices to the state before the last call to enable2DMode().
void disable2DMode(RenderingContext & rc);

}

#endif /* DRAW_H_ */

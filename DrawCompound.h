/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_DRAWCOMPOUND_H_
#define RENDERING_DRAWCOMPOUND_H_

/**
 * @file
 * Draw functions for compound objects
 */

// Forward declarations
namespace Geometry {
class Frustum;
}
namespace Rendering {
class RenderingContext;
}
namespace Util {
class Color4f;
}

namespace Rendering {

/** @addtogroup draw
 * @{
 */
 
//! Draw a symbolized camera using the given rendering context, and color.
void drawCamera(RenderingContext & rc, const Util::Color4f & color);

void drawCoordSys(RenderingContext & rc, float scale = 1.0f);

//! Draw the given frustum as lines using the given rendering context, color, and line width.
void drawFrustum(RenderingContext & rc, const Geometry::Frustum & frustum, const Util::Color4f & color, float lineWidth);

void drawGrid(RenderingContext & rc, float scale = 1.0f);

//! @}
}

#endif /* RENDERING_DRAWCOMPOUND_H_ */

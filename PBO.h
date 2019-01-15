/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_PBO_H
#define RENDERING_PBO_H

#include "BufferObject.h"
#include <Util/ReferenceCounter.h>
#include <cstdint>

namespace Rendering {
class Texture;

/**
* Representation of an OpenGL pixel buffer object (PBO).
*
* @note This implementation only supports asynchronous reading of pixel data from OpenGL to the application.
* @see http://www.opengl.org/registry/specs/ARB/pixel_buffer_object.txt
* @author Benjamin Eikel
* @date 2011-06-12
* @ingroup texture
*/
class PBO : public Util::ReferenceCounter<PBO> {
	private:
		//! OpenGL buffer object
		BufferObject bufferObject;

		//! Storage for the width of the last call to @a asynchronousReadPixels. Needed for size calculations.
		uint32_t width;

		//! Storage for the height of the last call to @a asynchronousReadPixels. Needed for size calculations.
		uint32_t height;

		//! Instruct OpenGL to allocate the data.
		void allocateBufferData(uint32_t newWidth, uint32_t newHeight);
	public:
		PBO();

		uint32_t getWidth() const {
			return width;
		}

		uint32_t getHeight() const {
			return height;
		}

		/**
		 * Activate the pixel buffer object and call @c glReadPixels.
		 * The call returns immediately and the pixel data is stored inside the pixel buffer object.
		 * The calling thread should continue processing before calling @a fillTexture.
		 *
		 * @see glReadPixels
		 * @param x X coordinate of the lower left pixel.
		 * @param y Y coordinate of the lower left pixel.
		 * @param width Horizontal dimension of the rectangle to read.
		 * @param height Vertical dimension of the rectangle to read.
		 */
		void asynchronousReadPixels(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

		/**
		 * Copy the data of this pixel buffer object into the given texture.
		 * The calling thread should wait a certain time before calling this function after calling @a asynchronousReadPixels.
		 * Otherwise there is no benefit in calling @c glReadPixels without using a pixel buffer object.
		 *
		 * @param texture A 2D texture with the same values for width and height as for the last call to @a asynchronousReadPixels.
		 * The format has to be @c GL_BGRA and the type has to be @c GL_UNSIGNED_BYTE.
		 * @return @c true if the data was copied successfully, @c false if the size or data format of the texture did not match or there is no data.
		 */
		bool fillTexture(Texture * texture) const;
};
}

#endif /* RENDERING_PBO_H */

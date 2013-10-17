/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#if defined(LIB_GL)

#include "PBO.h"
#include "Texture/Texture.h"
#include "BufferObject.h"
#include "GLHeader.h"
#include "Helper.h"
#include <Util/Graphics/Bitmap.h>
#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <cstdint>

namespace Rendering {

PBO::PBO() : bufferObject(), width(0), height(0) {
	if(!isExtensionSupported("GL_ARB_pixel_buffer_object")) {
		throw std::runtime_error("Fatal error: OpenGL extension GL_ARB_pixel_buffer_object is not supported.");
	}
}

void PBO::allocateBufferData(uint32_t newWidth, uint32_t newHeight) {
	const uint32_t numBytes = 4; // BGRA
	const uint32_t oldDataSize = numBytes * width * height;
	const uint32_t newDataSize = numBytes * newWidth * newHeight;

	width = newWidth;
	height = newHeight;

	if(newDataSize == oldDataSize || newDataSize == 0) {
		return;
	}

	bufferObject.allocateData<uint8_t>(GL_PIXEL_PACK_BUFFER, newDataSize, GL_STREAM_READ);
}

void PBO::asynchronousReadPixels(uint32_t x, uint32_t y, uint32_t _width, uint32_t _height) {
	allocateBufferData(_width, _height);

	bufferObject.bind(GL_PIXEL_PACK_BUFFER);
	glReadPixels(static_cast<int>(x), static_cast<int>(y), static_cast<int>(_width), static_cast<int>(_height), GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
	bufferObject.unbind(GL_PIXEL_PACK_BUFFER);
}

bool PBO::fillTexture(Texture * texture) const {
	const Texture::Format & texFormat = texture->getFormat();
	if(texFormat.width != width || texFormat.height != height || texFormat.glFormat != GL_BGRA || texFormat.glDataType != GL_UNSIGNED_BYTE) {
		return false;
	}
	const uint32_t numBytes = 4 * width * height;
	texture->getLocalBitmap()->setData(bufferObject.downloadData<uint8_t>(GL_PIXEL_PACK_BUFFER, numBytes));
	return true;
}

}

#endif /* defined(LIB_GL) */


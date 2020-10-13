/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2014 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "Texture.h"
#include "../Core/Device.h"
#include "../Core/ImageStorage.h"

#include "../GLHeader.h"
#include "../BufferObject.h"
#include "../Helper.h"
#include "../RenderingContext/RenderingContext.h"

#include "TextureUtils.h"
#include <Util/Graphics/Bitmap.h>
#include <Util/Graphics/PixelFormat.h>
#include <Util/Macros.h>
#include <Util/References.h>
#include <Util/Graphics/PixelAccessor.h>
#include <Util/StringUtils.h>
#include <cstddef>
#include <iostream>
#include <cmath>


namespace Rendering {

// ----------------------------------------------------

Texture::Ref Texture::create(const DeviceRef& device, const Format& format) {
	return new Texture(device, format);
}
//--------------

Texture::Ref Texture::create(const DeviceRef& device, const ImageStorageRef& image) {
	Ref texture = new Texture(device, image->getFormat());
	texture->image = image;
	return texture;
}

//--------------

Texture::Texture(const DeviceRef& device, const Format& format) : device(device), format(format) {
	
	uint32_t dim = 0;
	if(format.extent.x() >= 1)
		++dim;
	else if(format.extent.x() < 0)
		dim = 4;

	if(format.extent.y() >= 1)
		++dim;
	else if(format.extent.y() < 0)
		dim = 4;

	if(format.extent.z() > 1)
		++dim;
	else if(format.extent.z() < 0)
		dim = 4;

	switch(dim) {
		case 1:
			tType = format.layers > 1 ? TextureType::TEXTURE_1D_ARRAY : TextureType::TEXTURE_1D;
			break;
		case 2:
			if(format.samples > 1)
				tType = TextureType::TEXTURE_2D_MULTISAMPLE;
			else
				tType = format.layers > 1 ? TextureType::TEXTURE_2D_ARRAY : TextureType::TEXTURE_2D;
			break;
		case 3:
			tType = TextureType::TEXTURE_3D;
			break;
		default:
			throw std::runtime_error("Texture: Unsupported texture type.");
	}

}

//--------------

//! [ctor]
Texture::Texture(Format _format) : Texture(Device::getDefault(), format) { }

//! [dtor]
Texture::~Texture() = default;

void Texture::_createGLID(RenderingContext & context) {
	if(image) {
		//INFO ("Recreating Texture!");
		if(isGLTextureValid()) {
			WARN("Recreating valid Texture!");
			removeGLData();
		}
	}

}

//---------------

void Texture::createMipmaps(RenderingContext & context) {
	if(!image || dataHasChanged)
		_uploadGLTexture(context);
	mipmapCreationIsPlanned = false;
}

//---------------

void Texture::_uploadGLTexture(RenderingContext & context, int level/*=0*/) {
	if(!image)
		_createGLID(context);
	dataHasChanged = false;
}

//---------------

void Texture::allocateLocalData() {
	if(localBitmap.isNotNull()) {
		WARN("Texture::allocateLocalData: Data already allocated");
		return;
	}
	/*const auto localFormat = TextureUtils::glPixelFormatToPixelFormat( format.pixelFormat );
	if(localFormat == Util::PixelFormat::UNKNOWN) {
		WARN("Texture::allocateLocalData: Unsupported pixel format.");
		localBitmap = nullptr;
	}else{
		localBitmap = new Util::Bitmap(getWidth(), getHeight()*getNumLayers(), localFormat);
	}*/
}

//---------------

bool Texture::isGLTextureValid() const {
	return image && image->getApiHandle();
}

//---------------

bool Texture::isGLTextureResident() const {
	return image && image->getApiHandle();
}

//---------------

void Texture::removeGLData() {
	image = nullptr;
}

//---------------

void Texture::clearGLData(const Util::Color4f& color) {
	throw std::runtime_error("Texture::clearGLData: unsupported.");
}

//---------------

void Texture::downloadGLTexture(RenderingContext & context) {
	if(!image) {
		WARN("downloadGLTexture: No glTexture available.");
		return;
	}
	dataHasChanged = false;

	if(!localBitmap)
		allocateLocalData();

	throw std::runtime_error("Texture::downloadGLTexture: unsupported.");
}

//---------------

uint8_t * Texture::getLocalData() { return localBitmap ? localBitmap->data() : nullptr; }

//---------------

const uint8_t * Texture::getLocalData() const { return localBitmap ? localBitmap->data() : nullptr; }

//---------------

uint8_t * Texture::openLocalData(RenderingContext & context) {
	if(!localBitmap) {
		allocateLocalData();
		if(getGLId()!=0)
			downloadGLTexture(context);
	}
	return getLocalData();
}

//---------------

void Texture::_setGLId(uint32_t _glId) {
	throw std::runtime_error("Texture::_setGLId: unsupported.");
}

//---------------

void Texture::enableComparision(RenderingContext & context, Comparison::function_t func) {
	throw std::runtime_error("Texture::enableComparision: unsupported.");
}

//---------------

}

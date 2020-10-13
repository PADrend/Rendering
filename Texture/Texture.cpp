/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2014 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "Texture.h"
#include "../Core/Device.h"
#include "../Core/ImageStorage.h"
#include "../Core/ImageView.h"
#include "../Core/Sampler.h"

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

Texture::Ref Texture::create(const DeviceRef& device, const Format& format, const SamplerRef& sampler) {
	return new Texture(device, format, sampler ? sampler : Sampler::create(device, {}));
}

//--------------

Texture::Ref Texture::create(const DeviceRef& device, const ImageStorageRef& image, const SamplerRef& sampler) {
	return create(device, ImageView::create(image, {image->getType(), 0u, image->getFormat().mipLevels, 0u, image->getFormat().layers}), sampler);
}
//--------------

Texture::Ref Texture::create(const DeviceRef& device, const ImageViewRef& view, const SamplerRef& sampler) {
	Ref texture = new Texture(device, view->getImage()->getFormat(), sampler ? sampler : Sampler::create(device, {}));
	texture->imageView = view;
	return texture;
}

//--------------

Texture::Texture(const DeviceRef& device, const Format& format, const SamplerRef& sampler) : device(device), format(format), sampler(sampler) {
	
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
Texture::Texture(Format _format) : Texture(Device::getDefault(), format, Sampler::create(Device::getDefault(), {})) { }

//! [dtor]
Texture::~Texture() = default;

void Texture::_createGLID(RenderingContext & context) {
	if(imageView) {
		//INFO ("Recreating Texture!");
		if(isGLTextureValid()) {
			WARN("Recreating valid Texture!");
			removeGLData();
		}
	}

}

//---------------

bool Texture::getUseLinearMinFilter() const {
	return sampler->getConfig().minFilter == ImageFilter::Linear;
}

//---------------

bool Texture::getUseLinearMagFilter() const {
	return sampler->getConfig().magFilter == ImageFilter::Linear;
}

//---------------

void Texture::createMipmaps(RenderingContext & context) {
	mipmapCreationIsPlanned = false;
}

//---------------

void Texture::upload() {
	if(imageView && !dataHasChanged)
		return;
	
	if(!imageView) {
		// allocate new image storage & create view
		auto image = ImageStorage::create(device, {format});
		if(!image) {
			WARN("Texture: Failed to allocate image storage.");
			return;
		}
		imageView = ImageView::create(image, {tType, 0u, format.mipLevels, 0u, format.layers});
		if(!imageView) {
			WARN("Texture: Failed to create image view.");
			return;
		}
	}

	if(localBitmap && dataHasChanged) {
		// TODO: upload data
		WARN("Texture: data upload is currently not supported.");
	}
	dataHasChanged = false;
}


//---------------

void Texture::_uploadGLTexture(RenderingContext & context, int level/*=0*/) {
	upload();
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

void Texture::removeGLData() {
	imageView = nullptr;
}

//---------------

void Texture::clearGLData(const Util::Color4f& color) {
	throw std::runtime_error("Texture::clearGLData: unsupported.");
}

//---------------

void Texture::downloadGLTexture(RenderingContext & context) {
	if(!imageView) {
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
		//if(isValid())
		//	download();
	}
	return getLocalData();
}

//---------------

const ImageStorageRef& Texture::getImage() const {
	static ImageStorageRef nullImage;
	return imageView ? imageView->getImage() : nullImage;
}

//---------------

uint32_t Texture::_prepareForBinding(RenderingContext & context){
	if(!isValid() || dataHasChanged)
		upload();
	//if(mipmapCreationIsPlanned)
	//	createMipmaps(context);
	return glId;
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

ResourceUsage Texture::getLastUsage() const {
	return imageView ? imageView->getLastUsage() : ResourceUsage::Undefined;
}

//---------------
}

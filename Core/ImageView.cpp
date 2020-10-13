/*
	This file is part of the Rendering library.
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "ImageView.h"
#include "ImageStorage.h"
#include "Device.h"

#include <Util/Macros.h>

#include <vulkan/vulkan.hpp>

namespace Rendering {

vk::Format getVkFormat(const InternalFormat& format);

//-------------

static vk::ImageViewType getViewType(TextureType type) {
	switch(type) {
		case TextureType::TEXTURE_1D: return vk::ImageViewType::e1D;
		case TextureType::TEXTURE_1D_ARRAY: return vk::ImageViewType::e1DArray;
		case TextureType::TEXTURE_2D: return vk::ImageViewType::e2D;
		case TextureType::TEXTURE_2D_ARRAY: return vk::ImageViewType::e2DArray;
		case TextureType::TEXTURE_3D: return vk::ImageViewType::e3D;
		case TextureType::TEXTURE_CUBE_MAP: return vk::ImageViewType::eCube;
		case TextureType::TEXTURE_CUBE_MAP_ARRAY: return vk::ImageViewType::eCubeArray;
		case TextureType::TEXTURE_BUFFER: return vk::ImageViewType::e1D;
		case TextureType::TEXTURE_2D_MULTISAMPLE: return vk::ImageViewType::e2D;
	}
	return vk::ImageViewType::e1D;
};

//---------------

static bool checkCompability(const ImageFormat& format, const TextureType& imageType, const ImageView::Configuration& config) {
	bool typeValid = true;
	bool layerCountValid = config.layerCount >= 1;
	bool baseLayerValid = true;
	bool mipLevelValid = true;
	switch(imageType) {
		case TextureType::TEXTURE_1D:
			switch(config.type) {
				case TextureType::TEXTURE_1D: layerCountValid = config.layerCount == 1; break;
				case TextureType::TEXTURE_1D_ARRAY: break;
				default: typeValid = false; break;
			}
			break;
		case TextureType::TEXTURE_2D:
			switch(config.type) {
				case TextureType::TEXTURE_2D_MULTISAMPLE:
				case TextureType::TEXTURE_2D: layerCountValid = config.layerCount == 1; break;
				case TextureType::TEXTURE_2D_ARRAY: break;
				case TextureType::TEXTURE_CUBE_MAP: layerCountValid = config.layerCount == 6; break;
				case TextureType::TEXTURE_CUBE_MAP_ARRAY: layerCountValid = (config.layerCount % 6) == 0; break;
				default: typeValid = false; break;
			}
			break;
		case TextureType::TEXTURE_3D:
			switch(config.type) {
				case TextureType::TEXTURE_3D: layerCountValid = config.layerCount == 1; baseLayerValid = config.baseLayer == 0; break;
				case TextureType::TEXTURE_2D:
				case TextureType::TEXTURE_2D_ARRAY: layerCountValid = config.layerCount == 1; mipLevelValid = config.mipLevelCount == 1; break;
				default: typeValid = false; break;
			}
			break;
		default:
			WARN("ImageView: image view type '" + getTypeString(config.type) + "' is not a valid view type.");
			return false;
	}
	if(!typeValid) {
		WARN("ImageView: image view type '" + getTypeString(config.type) + "' is not compatible with type '" + getTypeString(imageType) + "'");
		return false;
	}
	if(!layerCountValid) {
		WARN("ImageView: invalid layer count '" + std::to_string(config.layerCount) + "' for type '" + getTypeString(config.type) + "'");
		return false;
	}
	if(!baseLayerValid) {
		WARN("ImageView: invalid base layer '" + std::to_string(config.baseLayer) + "' for type '" + getTypeString(config.type) + "'");
		return false;
	}
	if(!mipLevelValid) {
		WARN("ImageView: invalid mip level count '" + std::to_string(config.mipLevelCount) + "' for type '" + getTypeString(config.type) + "'");
		return false;
	}
	return true;
};

//---------------

ImageView::Ref ImageView::create(const ImageStorageRef& image, const Configuration& config) {
	Ref obj = new ImageView(image, config);
	if(!obj->init()) {
		return nullptr;
	}
	return obj;
}

//---------------

ImageView::~ImageView() = default;

//---------------

ImageView::ImageView(const ImageStorageRef& image, const Configuration& config) : image(image), config(config) { }

//---------------

bool ImageView::init() {
	if(!image || !image->getApiHandle() || !checkCompability(image->getFormat(), image->getType(), config))
		return false;

	vk::Device vkDevice(image->getApiHandle());
	vk::Image vkImage(image->getApiHandle());

	auto type = getViewType(config.type);
	vk::Format format(static_cast<vk::Format>(getVkFormat(image->getFormat().pixelFormat)));
	
	auto view = vkDevice.createImageView({{}, 
		vkImage, type, format, {},
		{ vk::ImageAspectFlagBits::eColor,
			config.baseMipLevel, config.mipLevelCount,
			config.baseLayer, config.layerCount
		}
	});
	if(!view)
		return false;

	handle = ImageViewHandle::create(view, vkDevice);	
	return true;
}

//---------------

} /* Rendering */
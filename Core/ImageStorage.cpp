/*
	This file is part of the Rendering library.
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "ImageStorage.h"
#include "Device.h"

#include <Util/Macros.h>
#include <Util/StringUtils.h>

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>


namespace Rendering {

vk::Format getVkFormat(const InternalFormat& format);

//-------------

static VkImageType getImageType(TextureType type) {
	switch(type) {
		case TextureType::TEXTURE_1D: return VK_IMAGE_TYPE_1D;
		case TextureType::TEXTURE_2D: return VK_IMAGE_TYPE_2D;
		case TextureType::TEXTURE_3D: return VK_IMAGE_TYPE_3D;
		default: return VK_IMAGE_TYPE_MAX_ENUM; // never happens
	}
};

//-------------

static TextureType getTextureType(const Geometry::Vec3ui& extent) {
	uint32_t dim = 0;
	if(extent.x() >= 1)
		++dim;
	if(extent.y() >= 1)
		++dim;
	if(extent.z() > 1)
		++dim;

	switch(dim) {
		case 1: return TextureType::TEXTURE_1D;
		case 2: return TextureType::TEXTURE_2D;
		case 3: return TextureType::TEXTURE_3D;
		default: throw std::runtime_error("ImageStorage: Invalid extent (" + std::to_string(extent.x()) + "," + std::to_string(extent.y()) + "," + std::to_string(extent.z()) + ")");
	}
};

//-------------

static VkSampleCountFlagBits getSampleCount(uint32_t samples) {
	switch(samples) {
		case 1: return VK_SAMPLE_COUNT_1_BIT;
		case 2: return VK_SAMPLE_COUNT_2_BIT;
		case 4: return VK_SAMPLE_COUNT_4_BIT;
		case 8: return VK_SAMPLE_COUNT_8_BIT;
		case 16: return VK_SAMPLE_COUNT_16_BIT;
		case 32: return VK_SAMPLE_COUNT_32_BIT;
		case 64: return VK_SAMPLE_COUNT_64_BIT;
		default: return VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM;
	}
};

//-------------

ImageStorage::Ref ImageStorage::create(const DeviceRef& device, const ImageStorage::Configuration& config) {
	Ref image(new ImageStorage(device, config));
	if(!image->init()) {
		WARN("ImageStorage: failed to allocate image with dimensions (" 
			+ Util::StringUtils::toString(config.format.extent.x()) + ","
			+ Util::StringUtils::toString(config.format.extent.y()) + ","
			+ Util::StringUtils::toString(config.format.extent.z()) + ")."
		);
		return nullptr;
	}
	return std::move(image);
}

//-------------

ImageStorage::Ref ImageStorage::createFromHandle(const DeviceRef& device, const Configuration& config, ImageHandle&& handle) {
	Ref image(new ImageStorage(device, config));
	image->handle = std::move(handle);
	return std::move(image);
}

//-------------

ImageStorage::ImageStorage(const DeviceRef& device, const ImageStorage::Configuration& config) : device(device), config(config), type(getTextureType(config.format.extent)) { }

//-------------

bool ImageStorage::init() {
	VkImage vkImage = nullptr;
	VkImageCreateInfo imageCreateInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	imageCreateInfo.flags = 0;
	imageCreateInfo.imageType = getImageType(type);
	imageCreateInfo.format = static_cast<VkFormat>(getVkFormat(config.format.pixelFormat));
	imageCreateInfo.extent = {
		static_cast<uint32_t>(config.format.extent.x()), 
		static_cast<uint32_t>(config.format.extent.y()), 
		static_cast<uint32_t>(config.format.extent.z())
	};
	imageCreateInfo.mipLevels = config.format.mipLevels;
	imageCreateInfo.arrayLayers = config.format.layers;
	imageCreateInfo.samples = getSampleCount(config.format.samples);
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
	imageCreateInfo.usage = config.usageFlags;

	if(imageCreateInfo.imageType == VK_IMAGE_TYPE_MAX_ENUM) {		
		WARN("ImageStorage: invalid image extent.");
		return false;
	}

	if(imageCreateInfo.format == VK_FORMAT_UNDEFINED) {		
		WARN("ImageStorage: invalid image format.");
		return false;
	}

	if(imageCreateInfo.samples == VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM) {		
		WARN("ImageStorage: invalid sample count: " + Util::StringUtils::toString(config.format.samples));
		return false;
	}

	VmaAllocationCreateInfo allocCreateInfo{};
	switch(config.memoryUsage) {
		case MemoryUsage::CpuOnly: allocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY; break;
		case MemoryUsage::GpuOnly: allocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY; break;
		case MemoryUsage::CpuToGpu: allocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU; break;
		case MemoryUsage::GpuToCpu: allocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_TO_CPU; break;
		default: allocCreateInfo.usage = VMA_MEMORY_USAGE_UNKNOWN; break;
	}

	if(config.usageFlags & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) {
		allocCreateInfo.preferredFlags = VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
	}

	VmaAllocation vmaAllocation = nullptr;
	VmaAllocationInfo allocationInfo{};
	if(vmaCreateImage(device->getAllocator(), &imageCreateInfo, &allocCreateInfo, &vkImage, &vmaAllocation, &allocationInfo) != VK_SUCCESS)
		return false;
	
	handle = ImageHandle::create(vkImage, device->getApiHandle());
	allocation = AllocationHandle::create(vmaAllocation, device->getAllocator());
	return true;
}

//-------------
} /* Rendering */
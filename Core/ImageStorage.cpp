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

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>


namespace Rendering {

vk::Format getVkFormat(const InternalFormat& format);
vk::ImageUsageFlags getVkImageUsage(const ResourceUsage& usage);

//-------------

static vk::ImageType getImageType(TextureType type) {
	switch(type) {
		case TextureType::TEXTURE_1D: return vk::ImageType::e1D;
		case TextureType::TEXTURE_2D: return vk::ImageType::e2D;
		case TextureType::TEXTURE_3D: return vk::ImageType::e3D;
		default: return {}; // never happens
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

static vk::SampleCountFlagBits getSampleCount(uint32_t samples) {
	switch(samples) {
		case 1: return vk::SampleCountFlagBits::e1;
		case 2: return vk::SampleCountFlagBits::e2;
		case 4: return vk::SampleCountFlagBits::e4;
		case 8: return vk::SampleCountFlagBits::e8;
		case 16: return vk::SampleCountFlagBits::e16;
		case 32: return vk::SampleCountFlagBits::e32;
		case 64: return vk::SampleCountFlagBits::e64;
		default: return {};
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
	vk::ImageCreateInfo imageCreateInfo{};
	imageCreateInfo.imageType = getImageType(type);
	imageCreateInfo.format = getVkFormat(config.format.pixelFormat);
	imageCreateInfo.extent.width = static_cast<uint32_t>(config.format.extent.x());
	imageCreateInfo.extent.height = static_cast<uint32_t>(config.format.extent.y());
	imageCreateInfo.extent.depth = static_cast<uint32_t>(config.format.extent.z());
	imageCreateInfo.mipLevels = config.format.mipLevels;
	imageCreateInfo.arrayLayers = config.format.layers;
	imageCreateInfo.samples = getSampleCount(config.format.samples);
	imageCreateInfo.tiling = vk::ImageTiling::eOptimal;
	imageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
	imageCreateInfo.usage = getVkImageUsage(config.usage);

	
	std::vector<uint32_t> familyIndices;
	auto queues = device->getQueues();
	for(auto& queue : device->getQueues()) {
		familyIndices.emplace_back(queue->getFamilyIndex());
	}
	if(familyIndices.size() > 1) {
		imageCreateInfo.sharingMode = vk::SharingMode::eConcurrent;
	}

	if(type != TextureType::TEXTURE_1D && type != TextureType::TEXTURE_2D && type != TextureType::TEXTURE_3D) {
		WARN("ImageStorage: invalid image type.");
		return false;
	}

	if(imageCreateInfo.format == vk::Format::eUndefined) {
		WARN("ImageStorage: invalid image format.");
		return false;
	}

	if(config.format.samples != 1 && config.format.samples != 2 && config.format.samples != 4 && 
		config.format.samples != 8 && config.format.samples != 16 && config.format.samples != 32 && config.format.samples != 64
	) {
		WARN("ImageStorage: invalid sample count: " + Util::StringUtils::toString(config.format.samples));
		return false;
	}

	vk::PhysicalDevice physicalDevice(device->getApiHandle());
	vk::ImageFormatProperties imageProperties;
	vk::Result result = physicalDevice.getImageFormatProperties(imageCreateInfo.format, imageCreateInfo.imageType, imageCreateInfo.tiling, imageCreateInfo.usage, imageCreateInfo.flags, &imageProperties);
	WARN_AND_RETURN_IF(result != vk::Result::eSuccess, "ImageStorage: invalid combination of format, type, and usage.", false);

	VmaAllocationCreateInfo allocCreateInfo{};
	switch(config.access) {
		case MemoryUsage::CpuOnly: allocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY; break;
		case MemoryUsage::GpuOnly: allocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY; break;
		case MemoryUsage::CpuToGpu: allocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU; break;
		case MemoryUsage::GpuToCpu: allocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_TO_CPU; break;
		default: allocCreateInfo.usage = VMA_MEMORY_USAGE_UNKNOWN; break;
	}

	if(imageCreateInfo.usage & vk::ImageUsageFlagBits::eTransientAttachment) {
		allocCreateInfo.preferredFlags = VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
	}

	VmaAllocation vmaAllocation = nullptr;
	VmaAllocationInfo allocationInfo{};
	VkImageCreateInfo vkImageCreateInfo = imageCreateInfo;
	if(vmaCreateImage(device->getAllocator(), &vkImageCreateInfo, &allocCreateInfo, &vkImage, &vmaAllocation, &allocationInfo) != VK_SUCCESS)
		return false;
	
	handle = ImageHandle::create(vkImage, device->getApiHandle());
	allocation = AllocationHandle::create(vmaAllocation, device->getAllocator());
	dataSize = allocationInfo.size;
	return true;
}

//-------------

void ImageStorage::setDebugName(const std::string& name) {
	if(!device->getConfig().debugMode)
		return;
	vk::Device vkDevice(device->getApiHandle());
	vkDevice.setDebugUtilsObjectNameEXT({ vk::Image::objectType, handle, name.c_str() });
}

//-------------
} /* Rendering */
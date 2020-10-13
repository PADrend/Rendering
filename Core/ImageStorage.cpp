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

//-------------

static VkImageType getImageType(const Geometry::Vec3i& extent) {
	uint32_t dim = 0;
	if(extent.x() >= 1)
		++dim;
	else if(extent.x() < 0)
		dim = 4;

	if(extent.y() >= 1)
		++dim;
	else if(extent.y() < 0)
		dim = 4;

	if(extent.z() > 1)
		++dim;
	else if(extent.z() < 0)
		dim = 4;

	switch(dim) {
		case 1: return VK_IMAGE_TYPE_1D;
		case 2: return VK_IMAGE_TYPE_2D;
		case 3: return VK_IMAGE_TYPE_3D;
		default: return VK_IMAGE_TYPE_MAX_ENUM;
	}
};

//-------------

static VkFormat convertFormat(const PixelFormat& format) {
	switch(format) {
		case PixelFormat::R8Unorm: return VK_FORMAT_R8_UNORM;
		case PixelFormat::R8Snorm: return VK_FORMAT_R8_SNORM;
		case PixelFormat::R16Unorm: return VK_FORMAT_R16_UNORM;
		case PixelFormat::R16Snorm: return VK_FORMAT_R16_SNORM;
		case PixelFormat::RG8Unorm: return VK_FORMAT_R8G8_UNORM;
		case PixelFormat::RG8Snorm: return VK_FORMAT_R8G8_SNORM;
		case PixelFormat::RG16Unorm: return VK_FORMAT_R16G16_UNORM;
		case PixelFormat::RG16Snorm: return VK_FORMAT_R16G16_SNORM;
		case PixelFormat::RGB16Unorm: return VK_FORMAT_R16G16B16_UNORM;
		case PixelFormat::RGB16Snorm: return VK_FORMAT_R16G16B16_SNORM;
		case PixelFormat::RGB5A1Unorm: return VK_FORMAT_A1R5G5B5_UNORM_PACK16;
		case PixelFormat::RGBA8Unorm: return VK_FORMAT_R8G8B8A8_UNORM;
		case PixelFormat::RGBA8Snorm: return VK_FORMAT_R8G8B8A8_SNORM;
		case PixelFormat::RGB10A2Unorm: return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
		case PixelFormat::RGB10A2Uint: return VK_FORMAT_A2R10G10B10_UINT_PACK32;
		case PixelFormat::RGBA16Unorm: return VK_FORMAT_R16G16B16A16_UNORM;
		case PixelFormat::RGBA8UnormSrgb: return VK_FORMAT_R8G8B8A8_SRGB;
		case PixelFormat::R16Float: return VK_FORMAT_R16_SFLOAT;
		case PixelFormat::RG16Float: return VK_FORMAT_R16G16_SFLOAT;
		case PixelFormat::RGB16Float: return VK_FORMAT_R16G16B16_SFLOAT;
		case PixelFormat::RGBA16Float: return VK_FORMAT_R16G16B16A16_SFLOAT;
		case PixelFormat::R32Float: return VK_FORMAT_R32_SFLOAT;
		case PixelFormat::RG32Float: return VK_FORMAT_R32G32_SFLOAT;
		case PixelFormat::RGB32Float: return VK_FORMAT_R32G32B32_SFLOAT;
		case PixelFormat::RGBA32Float: return VK_FORMAT_R32G32B32A32_SFLOAT;
		case PixelFormat::R11G11B10Float: return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
		case PixelFormat::RGB9E5Float: return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;
		case PixelFormat::R8Int: return VK_FORMAT_R8_SINT;
		case PixelFormat::R8Uint: return VK_FORMAT_R8_UINT;
		case PixelFormat::R16Int: return VK_FORMAT_R16_SINT;
		case PixelFormat::R16Uint: return VK_FORMAT_R16_UINT;
		case PixelFormat::R32Int: return VK_FORMAT_R32_SINT;
		case PixelFormat::R32Uint: return VK_FORMAT_R32_UINT;
		case PixelFormat::RG8Int: return VK_FORMAT_R8G8_SINT;
		case PixelFormat::RG8Uint: return VK_FORMAT_R8G8_UINT;
		case PixelFormat::RG16Int: return VK_FORMAT_R16G16_SINT;
		case PixelFormat::RG16Uint: return VK_FORMAT_R16G16_UINT;
		case PixelFormat::RG32Int: return VK_FORMAT_R32G32_SINT;
		case PixelFormat::RG32Uint: return VK_FORMAT_R32G32_UINT;
		case PixelFormat::RGB16Int: return VK_FORMAT_R16G16B16_SINT;
		case PixelFormat::RGB16Uint: return VK_FORMAT_R16G16B16_UINT;
		case PixelFormat::RGB32Int: return VK_FORMAT_R32G32B32_SINT;
		case PixelFormat::RGB32Uint: return VK_FORMAT_R32G32B32_UINT;
		case PixelFormat::RGBA8Int: return VK_FORMAT_R8G8B8A8_SINT;
		case PixelFormat::RGBA8Uint: return VK_FORMAT_R8G8B8A8_UINT;
		case PixelFormat::RGBA16Int: return VK_FORMAT_R16G16B16A16_SINT;
		case PixelFormat::RGBA16Uint: return VK_FORMAT_R16G16B16A16_UINT;
		case PixelFormat::RGBA32Int: return VK_FORMAT_R32G32B32A32_SINT;
		case PixelFormat::RGBA32Uint: return VK_FORMAT_R32G32B32A32_UINT;
		case PixelFormat::BGRA8Unorm: return VK_FORMAT_B8G8R8A8_UNORM;
		case PixelFormat::BGRA8UnormSrgb: return VK_FORMAT_B8G8R8A8_SRGB;
		case PixelFormat::R5G6B5Unorm: return VK_FORMAT_R5G6B5_UNORM_PACK16;
		case PixelFormat::D32Float: return VK_FORMAT_D32_SFLOAT;
		case PixelFormat::D16Unorm: return VK_FORMAT_D16_UNORM;
		case PixelFormat::D32FloatS8X24: return VK_FORMAT_D32_SFLOAT_S8_UINT;
		case PixelFormat::D24UnormS8: return VK_FORMAT_D24_UNORM_S8_UINT;
		case PixelFormat::BC1Unorm: return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
		case PixelFormat::BC1UnormSrgb: return VK_FORMAT_BC1_RGB_SRGB_BLOCK;
		case PixelFormat::BC2Unorm: return VK_FORMAT_BC2_UNORM_BLOCK;
		case PixelFormat::BC2UnormSrgb: return VK_FORMAT_BC2_SRGB_BLOCK;
		case PixelFormat::BC3Unorm: return VK_FORMAT_BC3_UNORM_BLOCK;
		case PixelFormat::BC3UnormSrgb: return VK_FORMAT_BC3_SRGB_BLOCK;
		case PixelFormat::BC4Unorm: return VK_FORMAT_BC4_UNORM_BLOCK;
		case PixelFormat::BC4Snorm: return VK_FORMAT_BC4_SNORM_BLOCK;
		case PixelFormat::BC5Unorm: return VK_FORMAT_BC5_UNORM_BLOCK;
		case PixelFormat::BC5Snorm: return VK_FORMAT_BC5_SNORM_BLOCK;
		case PixelFormat::BC6HS16: return VK_FORMAT_BC6H_SFLOAT_BLOCK;
		case PixelFormat::BC6HU16: return VK_FORMAT_BC6H_UFLOAT_BLOCK;
		case PixelFormat::BC7Unorm: return VK_FORMAT_BC7_UNORM_BLOCK;
		case PixelFormat::BC7UnormSrgb: return VK_FORMAT_BC7_SRGB_BLOCK;
		default: return VK_FORMAT_UNDEFINED;
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

ImageStorage::ImageStorage(const DeviceRef& device, const ImageStorage::Configuration& config) : device(device), config(config) { }

//-------------

bool ImageStorage::init() {
	VkImage vkImage = nullptr;
	VkImageCreateInfo imageCreateInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	imageCreateInfo.flags = 0;
	imageCreateInfo.imageType = getImageType(config.format.extent);
	imageCreateInfo.format = convertFormat(config.format.pixelFormat);
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
		case CpuOnly: allocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY; break;
		case GpuOnly: allocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY; break;
		case CpuToGpu: allocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU; break;
		case GpuToCpu: allocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_TO_CPU; break;
		default: allocCreateInfo.usage = VMA_MEMORY_USAGE_UNKNOWN; break;
	}

	if(config.usageFlags & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) {
		allocCreateInfo.preferredFlags = VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
	}

	VmaAllocation vmaAllocation = nullptr;
	VmaAllocationInfo allocationInfo{};
	if(vmaCreateImage(device->getAllocator(), &imageCreateInfo, &allocCreateInfo, &vkImage, &vmaAllocation, &allocationInfo) != VK_SUCCESS)
		return false;
	
	handle = std::move(ImageHandle(vkImage, device->getApiHandle()));
	allocation = std::move(AllocationHandle(vmaAllocation, device->getAllocator()));
	return true;
}

//-------------
} /* Rendering */
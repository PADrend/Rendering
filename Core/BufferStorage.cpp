/*
	This file is part of the Rendering library.
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "BufferStorage.h"
#include "Device.h"

#include <Util/Macros.h>
#include <Util/StringUtils.h>

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace Rendering {

//-------------

vk::BufferUsageFlags getVkBufferUsage(const ResourceUsage& usage);

//-------------

BufferStorage::Ref BufferStorage::create(const DeviceRef& device, const BufferStorage::Configuration& config) {
	Ref buffer(new BufferStorage(device, config));
	if(!buffer->init()) {
		WARN("BufferStorage: failed to allocate buffer of size " + Util::StringUtils::toString(config.size) + " bytes.");
		return nullptr;
	}
	return std::move(buffer);
}

//-------------

BufferStorage::BufferStorage(const DeviceRef& device, const BufferStorage::Configuration& config) : device(device), config(config) { }

//-------------

BufferStorage::~BufferStorage() {
	unmap();
}

//-------------

void BufferStorage::flush() const {
	vmaFlushAllocation(device->getAllocator(), allocation, 0, config.size);
}

//-------------

uint8_t* BufferStorage::map() {
	if(!mappedPtr) {
		if(vmaMapMemory(device->getAllocator(), allocation, reinterpret_cast<void **>(&mappedPtr)) != VK_SUCCESS) {
			WARN("BufferStorage: could not map memory.");
			mappedPtr = nullptr;
		}
	}
	return mappedPtr;
}

//-------------

void BufferStorage::unmap() {
	if(mappedPtr && !config.persistent)
		vmaUnmapMemory(device->getAllocator(), allocation);
}

//-------------

void BufferStorage::upload(const uint8_t* data, size_t size, size_t offset) {
	if(size+offset > config.size) {
		WARN("BufferStorage: upload size exceeds buffer size.");
		return;
	}
	if(config.persistent) {
		std::copy(data, data+size, mappedPtr+offset);
		flush();
	} else {
		map();
		if(!mappedPtr) {
			WARN("BufferStorage: failed to upload data.");
			return;
		}
		std::copy(data, data+size, mappedPtr+offset);
		flush();
		unmap();
	}
}

//-------------

bool BufferStorage::init() {
	VkBuffer vkBuffer = nullptr;
	VkBufferCreateInfo bufferCreateInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	bufferCreateInfo.usage = static_cast<uint32_t>(getVkBufferUsage(config.usage));
	bufferCreateInfo.size = config.size;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;

	VmaAllocationCreateInfo allocCreateInfo{};
	switch(config.access) {
		case MemoryUsage::CpuOnly: allocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY; break;
		case MemoryUsage::GpuOnly: allocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY; break;
		case MemoryUsage::CpuToGpu: allocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU; break;
		case MemoryUsage::GpuToCpu: allocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_TO_CPU; break;
		default: allocCreateInfo.usage = VMA_MEMORY_USAGE_UNKNOWN; break;
	}

	allocCreateInfo.flags = config.persistent ? VMA_ALLOCATION_CREATE_MAPPED_BIT : 0;

	VmaAllocation vmaAllocation = nullptr;
	VmaAllocationInfo allocationInfo{};
	if(vmaCreateBuffer(device->getAllocator(), &bufferCreateInfo, &allocCreateInfo, &vkBuffer, &vmaAllocation, &allocationInfo) != VK_SUCCESS)
		return false;

	if(config.persistent)
		mappedPtr = static_cast<uint8_t*>(allocationInfo.pMappedData);
	
	handle = std::move(BufferHandle(vkBuffer, device->getApiHandle()));
	allocation = std::move(AllocationHandle(vmaAllocation, device->getAllocator()));
	return true;
}

//-------------

} /* Rendering */
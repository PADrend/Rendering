/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	Copyright (C) 2014-2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "BufferObject.h"
#include "Core/Device.h"
#include "Core/BufferStorage.h"
#include "Core/Queue.h"
#include "Core/CommandBuffer.h"

#include <Util/Macros.h>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace Rendering {

//----------------

const uint32_t BufferObject::TARGET_ARRAY_BUFFER = 0;
const uint32_t BufferObject::TARGET_ATOMIC_COUNTER_BUFFER = 0;
const uint32_t BufferObject::TARGET_COPY_READ_BUFFER = 0;
const uint32_t BufferObject::TARGET_COPY_WRITE_BUFFER = 0;
const uint32_t BufferObject::TARGET_DISPATCH_INDIRECT_BUFFER = 0;
const uint32_t BufferObject::TARGET_DRAW_INDIRECT_BUFFER = 0;
const uint32_t BufferObject::TARGET_ELEMENT_ARRAY_BUFFER = 0;
const uint32_t BufferObject::TARGET_PIXEL_PACK_BUFFER = 0;
const uint32_t BufferObject::TARGET_PIXEL_UNPACK_BUFFER = 0;
const uint32_t BufferObject::TARGET_QUERY_BUFFER = 0;
const uint32_t BufferObject::TARGET_SHADER_STORAGE_BUFFER = 0;
const uint32_t BufferObject::TARGET_TEXTURE_BUFFER = 0;
const uint32_t BufferObject::TARGET_TRANSFORM_FEEDBACK_BUFFER = 0;
const uint32_t BufferObject::TARGET_UNIFORM_BUFFER = 0;

const uint32_t BufferObject::USAGE_STREAM_DRAW = 0;
const uint32_t BufferObject::USAGE_STREAM_READ = 0;
const uint32_t BufferObject::USAGE_STREAM_COPY = 0;
const uint32_t BufferObject::USAGE_STATIC_DRAW = 0;
const uint32_t BufferObject::USAGE_STATIC_READ = 0;
const uint32_t BufferObject::USAGE_STATIC_COPY = 0;
const uint32_t BufferObject::USAGE_DYNAMIC_DRAW = 0;
const uint32_t BufferObject::USAGE_DYNAMIC_READ = 0;
const uint32_t BufferObject::USAGE_DYNAMIC_COPY = 0;

//----------------

BufferObject::Ref BufferObject::create(const DeviceRef& device) {
	return new BufferObject(device);
}

//----------------

BufferObject::BufferObject() : BufferObject(Device::getDefault()) { }

//----------------


BufferObject::BufferObject(const DeviceRef& device) : device(device) { }

//----------------

BufferObject::~BufferObject() = default;

//----------------

void BufferObject::swap(BufferObject& other) {
	device.swap(other.device);
	buffer.swap(other.buffer);
}

//----------------

void BufferObject::destroy() {
	buffer = nullptr;
}

//----------------

bool BufferObject::allocate(size_t size, ResourceUsage usage, MemoryUsage access, bool persistent) {
	BufferStorage::Configuration config{size, access, persistent, usage};
	if(buffer && config == buffer->getConfig())
		return true; // already allocated.
	buffer = BufferStorage::create(device, config);
	return isValid();
}

//----------------

void BufferObject::upload(const uint8_t* data, size_t numBytes, size_t offset) {
	WARN_AND_RETURN_IF(!isValid(), "BufferObject: Cannot upload data. Buffer is not allocated.",);
	WARN_AND_RETURN_IF(buffer->getSize() < offset+numBytes, "BufferObject: Cannot upload data. Range out of bounds.",);
	if(buffer->isMappable()) {
		buffer->upload(data, numBytes, offset);
	} else {
		uint8_t* ptr = map();
		std::copy(data, data+numBytes, ptr+offset);
		unmap();
	}
}

//----------------

std::vector<uint8_t> BufferObject::download(size_t range, size_t offset) {
	WARN_AND_RETURN_IF(!isValid(), "BufferObject: Cannot download data. Buffer is not allocated.", {});
	WARN_AND_RETURN_IF(buffer->getSize() < range+offset, "BufferObject: Cannot download data. Range out of bounds.", {});
	const uint8_t* ptr = map();
	const std::vector<uint8_t> result(ptr + offset, ptr + offset + range);
	unmap();
	return result;
}

//----------------


void BufferObject::clear(uint32_t bufferTarget, uint32_t internalFormat, uint32_t format, uint32_t type, const uint8_t* data) {
	WARN("BufferObject::clear not supported!");
}

//----------------

void BufferObject::clear(uint32_t internalFormat, uint32_t format, uint32_t type, const uint8_t* data) {
	WARN("BufferObject::clear not supported!");
}

//----------------

void BufferObject::copy(const BufferObject& source, uint32_t sourceOffset, uint32_t targetOffset, uint32_t size) {
	WARN("BufferObject::copy not supported!");
}

//----------------

uint8_t* BufferObject::map() {
	if(!buffer)
		return nullptr;
	if(buffer->isMappable())
		return buffer->map();
	
	// Create & map staging buffer
	if(!stagingBuffer) {
		static uint32_t stagingBufferId = 0;
		stagingBuffer = BufferStorage::create(device, {buffer->getSize(), MemoryUsage::CpuOnly, false, ResourceUsage::CopySource});
		stagingBuffer->setDebugName("Staging Buffer " + std::to_string(stagingBufferId++));
		if(!stagingBuffer)
			return nullptr;
	}
	
	return stagingBuffer->map();
}

//----------------

void BufferObject::unmap() {
	if(!buffer)
		return;
	if(buffer->isMappable())
		return buffer->unmap();
	if(!stagingBuffer)
		return;
	
	stagingBuffer->unmap();
	CommandBuffer::Ref cmds = CommandBuffer::create(device->getQueue(QueueFamily::Transfer));
	cmds->copyBuffer(stagingBuffer, buffer, buffer->getSize());
	cmds->submit(true);
}

//----------------

size_t BufferObject::getSize() const {
	return buffer ? buffer->getSize() : 0;
}

//----------------

BufferHandle BufferObject::getApiHandle() const {
	return buffer ? buffer->getApiHandle() : nullptr;
}

//----------------
}

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
#include "../Core/Device.h"
#include "../Core/BufferStorage.h"
#include "../Core/Queue.h"
#include "../Core/CommandBuffer.h"

#include <Util/Macros.h>

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

BufferObject::Ref BufferObject::create(const DeviceRef& device, BufferAllocator::Ref allocator) {
	return new BufferObject(device, allocator);
}

//----------------

BufferObject::Ref BufferObject::create(const BufferStorageRef& buffer, size_t size, size_t offset, BufferAllocator::Ref allocator) {
	WARN_AND_RETURN_IF(!buffer || !buffer->getApiHandle(), "Cannot create BufferObject: Invalid buffer.", nullptr);
	size = size == 0 ? buffer->getSize() : size;
	WARN_AND_RETURN_IF(size+offset > buffer->getSize(), "Cannot create BufferObject: offset+size exceeds buffer size.", nullptr);
	Ref bo = new BufferObject(buffer->getDevice(), allocator);
	bo->offset = offset;
	bo->size = size;
	bo->buffer = buffer;
	return bo;
}

//----------------

BufferObject::BufferObject() : BufferObject(Device::getDefault()) { }

//----------------


BufferObject::BufferObject(const DeviceRef& device, BufferAllocator::Ref allocator) : device(device), allocator(allocator) { }

//----------------

BufferObject::~BufferObject() {
	destroy();
}

//----------------

void BufferObject::swap(BufferObject& other) {
	device.swap(other.device);
	buffer.swap(other.buffer);
	stagingBuffer.swap(other.stagingBuffer);
	std::swap(size, other.size);
	std::swap(offset, other.offset);
	std::swap(allocator, other.allocator);
}

//----------------

void BufferObject::destroy() {
	if(allocator)
		allocator->free(this);
	buffer = nullptr;
	stagingBuffer = nullptr;
	size = 0;
	offset = 0;
	allocator = nullptr;
}

//----------------

bool BufferObject::allocate(size_t _size, ResourceUsage usage, MemoryUsage access, bool persistent) {
	BufferStorage::Configuration config{_size, access, persistent, usage};
	if(buffer && config == buffer->getConfig())
		return true; // already allocated.
	destroy();
	buffer = BufferStorage::create(device, config);
	offset = 0;
	size = _size;
	stagingBuffer = nullptr;
	return isValid();
}

//----------------

void BufferObject::upload(const uint8_t* data, size_t numBytes, size_t _offset) {
	WARN_AND_RETURN_IF(!isValid(), "BufferObject: Cannot upload data. Buffer is not allocated.",);
	WARN_AND_RETURN_IF(size < _offset+numBytes, "BufferObject: Cannot upload data. Range out of bounds.",);
	if(buffer->isMappable()) {
		buffer->upload(data, numBytes, offset+_offset);
	} else {
		uint8_t* ptr = map();
		std::copy(data, data+numBytes, ptr+_offset);
		unmap();
	}
}

//----------------

std::vector<uint8_t> BufferObject::download(size_t range, size_t _offset) {
	WARN_AND_RETURN_IF(!isValid(), "BufferObject: Cannot download data. Buffer is not allocated.", {});
	WARN_AND_RETURN_IF(size < range+offset, "BufferObject: Cannot download data. Range out of bounds.", {});
	const uint8_t* ptr = map();
	const std::vector<uint8_t> result(ptr + _offset, ptr + _offset + range);
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
		return buffer->map() + offset;
	
	// Create & map staging buffer
	if(!stagingBuffer) {
		static uint32_t stagingBufferId = 0;
		stagingBuffer = BufferStorage::create(device, {size, MemoryUsage::CpuOnly, false, ResourceUsage::CopySource});
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
	cmds->copyBuffer(stagingBuffer, buffer, buffer->getSize(), 0, offset);
	cmds->submit(true);
}

//----------------

BufferHandle BufferObject::getApiHandle() const {
	return buffer ? buffer->getApiHandle() : nullptr;
}

//----------------
}

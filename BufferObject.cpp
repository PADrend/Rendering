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

#include <Util/Macros.h>

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

void BufferObject::swap(BufferObject& other) {
	device.swap(other.device);
	buffer.swap(other.buffer);
}

//----------------

void BufferObject::destroy() {
	buffer = nullptr;
}

//----------------

bool BufferObject::allocate(size_t size, ResourceUsage usage = ResourceUsage::General, MemoryUsage access = MemoryUsage::CpuToGpu, bool persistent=false) {
	BufferStorage::Configuration config{size, access, persistent, usage};
	if(buffer && config == buffer->getConfig())
		return true; // already allocated.
	buffer = BufferStorage::create(device, config);
	return isValid();
}

//----------------

void BufferObject::upload(const uint8_t* data, size_t numBytes) {
	WARN_AND_RETURN_IF(!isValid(), "BufferObject: Cannot upload data. Buffer is not allocated.",);
	WARN_AND_RETURN_IF(buffer->getSize() > numBytes, "BufferObject: Cannot upload data. Range out of bounds.",);
	buffer->upload(data, numBytes);
	// TODO: use staging buffer if buffer is not client accessible
}

//----------------

std::vector<uint8_t> BufferObject::download(size_t range, size_t offset=0) {
	WARN_AND_RETURN_IF(!isValid(), "BufferObject: Cannot download data. Buffer is not allocated.", {});
	WARN_AND_RETURN_IF(buffer->getSize() > range+offset, "BufferObject: Cannot download data. Range out of bounds.", {});
	const uint8_t* ptr = map();
	const std::vector<uint8_t> result(ptr + offset, ptr + offset + range);
	unmap();
	// TODO: use staging buffer if buffer is not client accessible
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
	return buffer->map();
}

//----------------

void BufferObject::unmap() {
	if(!buffer)
		return;
	buffer->unmap();
}

//----------------

}

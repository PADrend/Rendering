/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "BufferObject.h"
#include "GLHeader.h"
#include "Helper.h"

#include <Util/Macros.h>

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace Rendering {

const uint32_t BufferObject::TARGET_ARRAY_BUFFER = GL_ARRAY_BUFFER;
const uint32_t BufferObject::TARGET_ATOMIC_COUNTER_BUFFER = GL_ATOMIC_COUNTER_BUFFER;
const uint32_t BufferObject::TARGET_COPY_READ_BUFFER = GL_COPY_READ_BUFFER;
const uint32_t BufferObject::TARGET_COPY_WRITE_BUFFER = GL_COPY_WRITE_BUFFER;
const uint32_t BufferObject::TARGET_DISPATCH_INDIRECT_BUFFER = GL_DISPATCH_INDIRECT_BUFFER;
const uint32_t BufferObject::TARGET_DRAW_INDIRECT_BUFFER = GL_DRAW_INDIRECT_BUFFER;
const uint32_t BufferObject::TARGET_ELEMENT_ARRAY_BUFFER = GL_ELEMENT_ARRAY_BUFFER;
const uint32_t BufferObject::TARGET_PIXEL_PACK_BUFFER = GL_PIXEL_PACK_BUFFER;
const uint32_t BufferObject::TARGET_PIXEL_UNPACK_BUFFER = GL_PIXEL_UNPACK_BUFFER;
const uint32_t BufferObject::TARGET_QUERY_BUFFER = GL_QUERY_BUFFER;
const uint32_t BufferObject::TARGET_SHADER_STORAGE_BUFFER = GL_SHADER_STORAGE_BUFFER;
const uint32_t BufferObject::TARGET_TEXTURE_BUFFER = GL_TEXTURE_BUFFER;
const uint32_t BufferObject::TARGET_TRANSFORM_FEEDBACK_BUFFER = GL_TRANSFORM_FEEDBACK_BUFFER;
const uint32_t BufferObject::TARGET_UNIFORM_BUFFER = GL_UNIFORM_BUFFER;

const uint32_t BufferObject::USAGE_STREAM_DRAW = GL_STREAM_DRAW;
const uint32_t BufferObject::USAGE_STREAM_READ = GL_STREAM_READ;
const uint32_t BufferObject::USAGE_STREAM_COPY = GL_STREAM_COPY;
const uint32_t BufferObject::USAGE_STATIC_DRAW = GL_STATIC_DRAW;
const uint32_t BufferObject::USAGE_STATIC_READ = GL_STATIC_READ;
const uint32_t BufferObject::USAGE_STATIC_COPY = GL_STATIC_COPY;
const uint32_t BufferObject::USAGE_DYNAMIC_DRAW = GL_DYNAMIC_DRAW;
const uint32_t BufferObject::USAGE_DYNAMIC_READ = GL_DYNAMIC_READ;
const uint32_t BufferObject::USAGE_DYNAMIC_COPY = GL_DYNAMIC_COPY;

BufferObject::BufferObject() : bufferId(0) {
}

BufferObject::BufferObject(BufferObject && other) : bufferId(other.bufferId) {
	// Make sure the other buffer object does not free the handle.
	other.bufferId = 0;
}

BufferObject::~BufferObject() {
	destroy();
}

BufferObject & BufferObject::operator=(BufferObject && other) {
	// Handle self-assignment.
	if(this == &other) {
		return *this;
	}
	// Make sure the other buffer object frees the handle.
	std::swap(other.bufferId, bufferId);
	return *this;
}

void BufferObject::swap(BufferObject & other){
	std::swap(other.bufferId,bufferId);
}

void BufferObject::prepare() {
	if(bufferId == 0) {
		glGenBuffers(1, &bufferId);
	}
}

void BufferObject::destroy() {
	if(bufferId != 0) {
		glDeleteBuffers(1, &bufferId);
		bufferId = 0;
	}
}

void BufferObject::bind(uint32_t bufferTarget) const {
	glBindBuffer(bufferTarget, bufferId);
}

void BufferObject::bind(uint32_t bufferTarget, uint32_t location) const {
#if defined(LIB_GL)
	glBindBufferBase(bufferTarget, location, bufferId);
#endif
}

void BufferObject::unbind(uint32_t bufferTarget) const {
	glBindBuffer(bufferTarget, 0);
}

void BufferObject::unbind(uint32_t bufferTarget, uint32_t location) const {
#if defined(LIB_GL)
	glBindBufferBase(bufferTarget, location, 0);
#endif
}

void BufferObject::uploadData(uint32_t bufferTarget, const uint8_t* data, size_t numBytes, uint32_t usageHint) {
	prepare();
	bind(bufferTarget);
	glBufferData(bufferTarget, static_cast<GLsizeiptr>(numBytes), data, usageHint);
	unbind(bufferTarget);
}

void BufferObject::uploadSubData(uint32_t bufferTarget, const uint8_t* data, size_t numBytes, size_t offset) {
	prepare();
	bind(bufferTarget);
	glBufferSubData(bufferTarget, offset, static_cast<GLsizeiptr>(numBytes), data);
	unbind(bufferTarget);
}

#if defined(LIB_GL)
std::vector<uint8_t> BufferObject::downloadData(uint32_t bufferTarget, std::size_t numBytes, size_t offset) const {
	if(bufferId == 0) {
		return std::vector<uint8_t>();
	}
	bind(bufferTarget);
	const uint8_t* bufferData = reinterpret_cast<const uint8_t*>(glMapBuffer(bufferTarget, GL_READ_ONLY));
	const std::vector<uint8_t> result(bufferData + offset, bufferData + offset + numBytes);
	glUnmapBuffer(bufferTarget);
	unbind(bufferTarget);
	return result;
}
#elif defined(LIB_GLESv2)
std::vector<uint8_t> BufferObject::downloadData(uint32_t /*bufferTarget*/, std::size_t /*numBytes*/, size_t /*offset*/) const {
	return std::vector<uint8_t>();
}
#endif

void BufferObject::clear(uint32_t bufferTarget, uint32_t internalFormat, uint32_t format, uint32_t type, const uint8_t* data) {
#if defined(GL_ARB_clear_buffer_object)
	bind(bufferTarget);
	glClearBufferData(bufferTarget, internalFormat, format, type, data);
	unbind(bufferTarget);
	//glClearNamedBufferDataEXT(bufferId, internalFormat, format, type, data);
#else
	WARN("BufferObject::clear not supported!");
#endif
}

void BufferObject::clear(uint32_t internalFormat, uint32_t format, uint32_t type, const uint8_t* data) {
#if defined(GL_ARB_clear_buffer_object)
	glClearNamedBufferDataEXT(bufferId, internalFormat, format, type, data);
#else
	WARN("BufferObject::clear not supported!");
#endif
}

void BufferObject::clear() {
	clear(GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT);
}

void BufferObject::copy(const BufferObject& source, uint32_t sourceOffset, uint32_t targetOffset, uint32_t size) {
	source.bind(TARGET_COPY_READ_BUFFER);
	bind(TARGET_COPY_WRITE_BUFFER);
	glCopyBufferSubData(TARGET_COPY_READ_BUFFER, TARGET_COPY_WRITE_BUFFER, sourceOffset, targetOffset, size);
	unbind(TARGET_COPY_WRITE_BUFFER);
	source.unbind(TARGET_COPY_READ_BUFFER);
	GET_GL_ERROR();
}

uint8_t* BufferObject::map(uint32_t offset, uint32_t size, AccessFlag access) {
	GLenum gl_access;
	switch(access) {
		case AccessFlag::READ_ONLY: gl_access = GL_READ_ONLY; break;
		case AccessFlag::WRITE_ONLY: gl_access = GL_WRITE_ONLY; break;
		case AccessFlag::READ_WRITE: gl_access = GL_READ_WRITE; break;
		default: 
			WARN("BufferObject::map: Cannot map buffer with access flag NO_ACCESS");
			return nullptr;
	}
	bind(TARGET_COPY_WRITE_BUFFER);
	uint8_t* ptr = nullptr;
	if(size == 0)
		ptr = static_cast<uint8_t*>(glMapBuffer(TARGET_COPY_WRITE_BUFFER, gl_access));
	else
		ptr = static_cast<uint8_t*>(glMapBufferRange(TARGET_COPY_WRITE_BUFFER, offset, size, gl_access));
	unbind(TARGET_COPY_WRITE_BUFFER);
	return ptr;
}

void BufferObject::unmap() {
	bind(TARGET_COPY_WRITE_BUFFER);
	glUnmapBuffer(TARGET_COPY_WRITE_BUFFER);
	unbind(TARGET_COPY_WRITE_BUFFER);
}

// Instantiate the template functions
template void BufferObject::allocateData<uint8_t>(uint32_t, std::size_t, uint32_t);
template void BufferObject::allocateData<uint16_t>(uint32_t, std::size_t, uint32_t);
template void BufferObject::allocateData<uint32_t>(uint32_t, std::size_t, uint32_t);
template void BufferObject::allocateData<uint64_t>(uint32_t, std::size_t, uint32_t);
template void BufferObject::allocateData<int8_t>(uint32_t, std::size_t, uint32_t);
template void BufferObject::allocateData<int16_t>(uint32_t, std::size_t, uint32_t);
template void BufferObject::allocateData<int32_t>(uint32_t, std::size_t, uint32_t);
template void BufferObject::allocateData<int64_t>(uint32_t, std::size_t, uint32_t);
template void BufferObject::allocateData<float>(uint32_t, std::size_t, uint32_t);
template void BufferObject::allocateData<double>(uint32_t, std::size_t, uint32_t);

template void BufferObject::uploadData<uint8_t>(uint32_t, const std::vector<uint8_t> &, uint32_t);
template void BufferObject::uploadData<uint16_t>(uint32_t, const std::vector<uint16_t> &, uint32_t);
template void BufferObject::uploadData<uint32_t>(uint32_t, const std::vector<uint32_t> &, uint32_t);
template void BufferObject::uploadData<uint64_t>(uint32_t, const std::vector<uint64_t> &, uint32_t);
template void BufferObject::uploadData<int8_t>(uint32_t, const std::vector<int8_t> &, uint32_t);
template void BufferObject::uploadData<int16_t>(uint32_t, const std::vector<int16_t> &, uint32_t);
template void BufferObject::uploadData<int32_t>(uint32_t, const std::vector<int32_t> &, uint32_t);
template void BufferObject::uploadData<int64_t>(uint32_t, const std::vector<int64_t> &, uint32_t);
template void BufferObject::uploadData<float>(uint32_t, const std::vector<float> &, uint32_t);
template void BufferObject::uploadData<double>(uint32_t, const std::vector<double> &, uint32_t);

template void BufferObject::uploadSubData<uint8_t>(uint32_t, const std::vector<uint8_t> &, size_t);
template void BufferObject::uploadSubData<uint16_t>(uint32_t, const std::vector<uint16_t> &, size_t);
template void BufferObject::uploadSubData<uint32_t>(uint32_t, const std::vector<uint32_t> &, size_t);
template void BufferObject::uploadSubData<uint64_t>(uint32_t, const std::vector<uint64_t> &, size_t);
template void BufferObject::uploadSubData<int8_t>(uint32_t, const std::vector<int8_t> &, size_t);
template void BufferObject::uploadSubData<int16_t>(uint32_t, const std::vector<int16_t> &, size_t);
template void BufferObject::uploadSubData<int32_t>(uint32_t, const std::vector<int32_t> &, size_t);
template void BufferObject::uploadSubData<int64_t>(uint32_t, const std::vector<int64_t> &, size_t);
template void BufferObject::uploadSubData<float>(uint32_t, const std::vector<float> &, size_t);
template void BufferObject::uploadSubData<double>(uint32_t, const std::vector<double> &, size_t);

template std::vector<uint8_t> BufferObject::downloadData<uint8_t>(uint32_t, size_t, size_t) const;
template std::vector<uint16_t> BufferObject::downloadData<uint16_t>(uint32_t, size_t, size_t) const;
template std::vector<uint32_t> BufferObject::downloadData<uint32_t>(uint32_t, size_t, size_t) const;
template std::vector<uint64_t> BufferObject::downloadData<uint64_t>(uint32_t, size_t, size_t) const;
template std::vector<int8_t> BufferObject::downloadData<int8_t>(uint32_t, size_t, size_t) const;
template std::vector<int16_t> BufferObject::downloadData<int16_t>(uint32_t, size_t, size_t) const;
template std::vector<int32_t> BufferObject::downloadData<int32_t>(uint32_t, size_t, size_t) const;
template std::vector<int64_t> BufferObject::downloadData<int64_t>(uint32_t, size_t, size_t) const;
template std::vector<float> BufferObject::downloadData<float>(uint32_t, size_t, size_t) const;
template std::vector<double> BufferObject::downloadData<double>(uint32_t, size_t, size_t) const;

}

/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	Copyright (C) 2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "BufferObject.h"
#include "../GLHeader.h"
#include "../Helper.h"
#include <Util/Macros.h>
#include <Util/StringUtils.h>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include <iostream>

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

const uint32_t BufferObject::FLAG_DYNAMIC_STORAGE = GL_DYNAMIC_STORAGE_BIT;
const uint32_t BufferObject::FLAG_MAP_READ = GL_MAP_READ_BIT;
const uint32_t BufferObject::FLAG_MAP_WRITE = GL_MAP_WRITE_BIT;
const uint32_t BufferObject::FLAG_MAP_PERSISTENT = GL_MAP_PERSISTENT_BIT;
const uint32_t BufferObject::FLAG_MAP_COHERENT = GL_MAP_COHERENT_BIT;
const uint32_t BufferObject::FLAG_CLIENT_STORAGE = GL_CLIENT_STORAGE_BIT;

const uint32_t BufferObject::FLAG_MAP_INVALIDATE_RANGE = GL_MAP_INVALIDATE_RANGE_BIT;
const uint32_t BufferObject::FLAG_MAP_INVALIDATE_BUFFFER = GL_MAP_INVALIDATE_BUFFER_BIT;
const uint32_t BufferObject::FLAG_MAP_FLUSH_EXPLICIT = GL_MAP_FLUSH_EXPLICIT_BIT;
const uint32_t BufferObject::FLAG_MAP_UNSYNCHRONIZED = GL_MAP_UNSYNCHRONIZED_BIT;

const uint32_t BufferObject::FLAGS_STATIC = 0;
const uint32_t BufferObject::FLAGS_DYNAMIC = GL_DYNAMIC_STORAGE_BIT;
const uint32_t BufferObject::FLAGS_PERSISTENT = GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT | GL_MAP_READ_BIT | GL_MAP_COHERENT_BIT;
const uint32_t BufferObject::FLAGS_STREAM = GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT | GL_MAP_COHERENT_BIT;

static uint32_t translateLegacyHint(uint32_t hint) {
	switch(hint) {
		case GL_STATIC_DRAW:
		case GL_STATIC_READ:
		case GL_STATIC_COPY:
			return BufferObject::FLAGS_STATIC;
		case GL_DYNAMIC_DRAW:
		case GL_DYNAMIC_READ:
		case GL_DYNAMIC_COPY:
			return BufferObject::FLAGS_DYNAMIC;
		case GL_STREAM_DRAW:
		case GL_STREAM_READ:
		case GL_STREAM_COPY:
			return BufferObject::FLAGS_PERSISTENT;
		default: return hint;
	}
}

static inline std::string bufferTargetName(uint32_t target) {
	switch(target) {
		case GL_SHADER_STORAGE_BUFFER: return "GL_SHADER_STORAGE_BUFFER";
		case GL_UNIFORM_BUFFER: return "GL_UNIFORM_BUFFER";
		case GL_ATOMIC_COUNTER_BUFFER: return "GL_ATOMIC_COUNTER_BUFFER";
		case GL_TRANSFORM_FEEDBACK_BUFFER: return "GL_TRANSFORM_FEEDBACK_BUFFER";
		default: return "UNKNOWN";
	}
}

inline bool isBitSet(uint32_t flags, uint32_t flag) {
	return flags & flag;
}

inline uint32_t extractStorageFlags(uint32_t flags) {
	return flags & ~(GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_FLUSH_EXPLICIT_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
}

inline uint32_t extractMapFlags(uint32_t flags) {
	return flags & ~GL_DYNAMIC_STORAGE_BIT & ~GL_CLIENT_STORAGE_BIT;
}

BufferObject::BufferObject(BufferObject && other) : bufferId(other.bufferId), flags(other.flags), size(other.size), ptr(other.ptr) {
	// Make sure the other buffer object does not free the handle.
	other.bufferId = 0;
	other.flags = 0;
	other.size = 0;
	other.ptr = nullptr;
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
	std::swap(other.size, size);
	std::swap(other.flags, flags);
	std::swap(other.ptr, ptr);
	return *this;
}

void BufferObject::swap(BufferObject & other){
	std::swap(other.bufferId,bufferId);
	std::swap(other.size, size);
	std::swap(other.flags, flags);
	std::swap(other.ptr, ptr);
}

void BufferObject::prepare() {
	if(bufferId == 0) {
		glCreateBuffers(1, &bufferId);
	}
}

void BufferObject::destroy() {
	if(bufferId != 0) {
		if(ptr)
			glUnmapNamedBuffer(bufferId);
		glDeleteBuffers(1, &bufferId);
		bufferId = 0;		
	}
	flags = 0;
	size = 0;
	ptr = nullptr;
}

void BufferObject::bind(uint32_t bufferTarget, uint32_t location, size_t offset, size_t rangeSize) const {
	const auto maxBindings = getMaxBufferBindings(bufferTarget);
	if(maxBindings <= location) {
		WARN("BufferObject::bind: Invalid binding location " + Util::StringUtils::toString(location) + 
				 " for buffer target '" + bufferTargetName(bufferTarget) +
				 "'. Maximum is " + Util::StringUtils::toString(maxBindings));
		return;
	} else if(maxBindings == 1) {
		glBindBuffer(bufferTarget, bufferId);
	} else if(offset > 0 || (rangeSize > 0 && rangeSize < size)) {
		glBindBufferRange(bufferTarget, location, bufferId, offset, rangeSize > 0 ? rangeSize : (size-offset));
	} else {
		glBindBufferBase(bufferTarget, location, bufferId);
	}
}

void BufferObject::unbind(uint32_t bufferTarget, uint32_t location) const {
	if(getMaxBufferBindings(bufferTarget) == 1)
		glBindBuffer(bufferTarget, 0);
	else
		glBindBufferBase(bufferTarget, location, 0);
}

void BufferObject::allocate(size_t numBytes, uint32_t hintOrFlags, const uint8_t* data) {
	prepare();
	uint32_t newFlags = translateLegacyHint(hintOrFlags);
	if(data && flags == newFlags && size == numBytes) {
		// only create new storage if flags or size changed
		upload(data, numBytes);
		return;
	}
	if(size > 0) {
		WARN("BufferObject: reallocating buffers is not allowed. Call destroy first to reallocate.");
		return;
	}
	
	flags = newFlags;
	size = numBytes;
	
	glNamedBufferStorage(bufferId, numBytes, data, extractStorageFlags(flags));
	
	if(isBitSet(flags, FLAG_MAP_PERSISTENT)) {
		// persistent mapped buffer
		ptr = static_cast<uint8_t*>(glMapNamedBufferRange(bufferId, 0, size, extractMapFlags(flags)));
	}
		
	GET_GL_ERROR();
}

void BufferObject::upload(const uint8_t* data, size_t numBytes, size_t offset) {
	if(size < offset+numBytes) {
		WARN("BufferObject::uploadSubData: buffer overflow!");
		return;
	}
	if(isBitSet(flags, FLAG_DYNAMIC_STORAGE)) {
		glNamedBufferSubData(bufferId, offset, numBytes, data);
	} else if(isBitSet(flags, FLAG_MAP_WRITE)) {
		if(isBitSet(flags, FLAG_MAP_PERSISTENT)) {
			std::copy(data, data + numBytes, ptr + offset);
		} else if(!ptr) {
			auto tmpPtr = static_cast<uint8_t*>(glMapNamedBufferRange(bufferId, offset, numBytes, GL_MAP_WRITE_BIT));
			std::copy(data, data + numBytes, tmpPtr);
			glUnmapNamedBuffer(bufferId);
		} else {
			WARN("BufferObject::uploadSubData: cannot upload data while buffer is mapped.");
		}
	} else {
		// use staging buffer
	  uint32_t stagingBuffer;
	  glCreateBuffers(1, &stagingBuffer);
  	glNamedBufferStorage(stagingBuffer, numBytes, data, 0);
  	glCopyNamedBufferSubData(stagingBuffer, bufferId, 0, offset, numBytes);
  	glDeleteBuffers(1, &stagingBuffer);
	}
	GET_GL_ERROR();
}

void BufferObject::download(uint8_t* targetPtr, size_t numBytes, size_t offset) const {
	if(bufferId == 0) {
		WARN("BufferObject::downloadData: invalid buffer!");
		return;
	}
	if(size < offset+numBytes) {
		std::cout << size << " " << offset << " " << numBytes << std::endl;
		WARN("BufferObject::downloadData: buffer overflow!");
		return;
	}
	if(isBitSet(flags, FLAG_MAP_READ)) {
		if(isBitSet(flags, FLAG_MAP_PERSISTENT)) {
			std::copy(ptr + offset, ptr + offset + numBytes, targetPtr);
		} else if(!ptr) {
			auto tmpPtr = static_cast<uint8_t*>(glMapNamedBufferRange(bufferId, offset, numBytes, GL_MAP_READ_BIT));
			std::copy(tmpPtr, tmpPtr + numBytes, targetPtr);
			glUnmapNamedBuffer(bufferId);
		} else {
			WARN("BufferObject::uploadSubData: cannot upload data while buffer is mapped.");
		}
	} else {
		glGetNamedBufferSubData(bufferId, offset, numBytes, targetPtr);
	}
	GET_GL_ERROR();
}

uint8_t* BufferObject::map(size_t offset, size_t range, uint32_t mapFlags) {
	if(mapFlags == 0)
		mapFlags = extractMapFlags(flags);
	if(range == 0)
		range = size - offset;
	if(bufferId == 0) {
		WARN("BufferObject::map: invalid buffer!");
	} else if(size < offset+range) {
		WARN("BufferObject::map: buffer overflow!");
	} else if(isBitSet(flags, FLAG_MAP_PERSISTENT)) {
		return ptr+offset;
	}	else if(ptr) {
		WARN("BufferObject::map: buffer already mapped!");
	} else if(isBitSet(flags, FLAG_MAP_READ | FLAG_MAP_WRITE)) {
		ptr = static_cast<uint8_t*>(glMapNamedBufferRange(bufferId, offset, range, mapFlags));
		GET_GL_ERROR();
		return ptr;
	} else {
		WARN("BufferObject::map: mapping is not allowed!");
	}
	return nullptr;
}

const uint8_t* BufferObject::map(size_t offset) const {
	if(bufferId == 0) {
		WARN("BufferObject::map: invalid buffer!");
	} else if(size <= offset) {
		WARN("BufferObject::map: buffer overflow!");
	} else if(!isBitSet(flags, FLAG_MAP_PERSISTENT)) {
		WARN("BufferObject::map: const mapping is not allowed for non-persistent buffers!");
	}	else {
		return ptr+offset;
	}
	return nullptr;
}

void BufferObject::unmap() {
	if(bufferId != 0 && ptr && !isBitSet(flags, FLAG_MAP_PERSISTENT))
		glUnmapNamedBuffer(bufferId);
}

void BufferObject::flush(size_t offset, size_t range) {
	if(isBitSet(flags, FLAG_MAP_WRITE)) {
		glFlushMappedNamedBufferRange(bufferId, offset, range == 0 ? size : range);
	}
}

void BufferObject::clear(uint32_t internalFormat, uint32_t format, uint32_t type, const uint8_t* data) {
	if(bufferId == 0)
		return;
	glClearNamedBufferData(bufferId, internalFormat, format, type, data);
	GET_GL_ERROR();
}

void BufferObject::copy(BufferObject& target, uint32_t dataSize, uint32_t srcOffset, uint32_t tgtOffset) {
	if(!isValid()) {
		WARN("BufferObject::copy: invalid source buffer.");
		return;
	}
	if(!target.isValid()) {
		WARN("BufferObject::copy: invalid target buffer.");
		return;
	}
	if(dataSize == 0 || srcOffset+dataSize > size || tgtOffset+dataSize > target.size) {
		WARN("BufferObject::copy: invalid offset+size.");
		return;
	}
	glCopyNamedBufferSubData(bufferId, target.bufferId, srcOffset, tgtOffset, dataSize);	
	GET_GL_ERROR();
}

}

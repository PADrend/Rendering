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
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace Rendering {


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

void BufferObject::unbind(uint32_t bufferTarget) const {
	glBindBuffer(bufferTarget, 0);
}

void BufferObject::uploadData(uint32_t bufferTarget, const uint8_t* data, size_t numBytes, uint32_t usageHint){
	prepare();
	bind(bufferTarget);
	glBufferData(bufferTarget, static_cast<GLsizeiptr>(numBytes), data, usageHint);
	unbind(bufferTarget);

}

template<typename T>
void BufferObject::allocateData(uint32_t bufferTarget, std::size_t numberOfElements, uint32_t usageHint) {
	uploadData(bufferTarget, nullptr, static_cast<GLsizeiptr>(numberOfElements * sizeof(T)), usageHint);
}

template<typename T>
void BufferObject::uploadData(uint32_t bufferTarget, const std::vector<T> & data, uint32_t usageHint) {
	uploadData(bufferTarget, reinterpret_cast<const uint8_t*>(data.data()),static_cast<GLsizeiptr>(data.size() * sizeof(T)),usageHint);
}

#if defined(LIB_GL)
template<typename T>
std::vector<T> BufferObject::downloadData(uint32_t bufferTarget, std::size_t numberOfElements) const {
	if(bufferId == 0) {
		return std::vector<T>();
	}
	bind(bufferTarget);
	const T * bufferData = reinterpret_cast<const T *>(glMapBuffer(bufferTarget, GL_READ_ONLY));
	const std::vector<T> result(bufferData, bufferData + numberOfElements);
	glUnmapBuffer(bufferTarget);
	unbind(bufferTarget);
	return result;
}
#elif defined(LIB_GLESv2)
template<typename T>
std::vector<T> BufferObject::downloadData(uint32_t /*bufferTarget*/, std::size_t /*numberOfElements*/) const {
	return std::vector<T>();
}
#endif

// Instantiate the template functions
template void BufferObject::allocateData<uint8_t>(uint32_t, std::size_t, uint32_t);
template void BufferObject::allocateData<uint32_t>(uint32_t, std::size_t, uint32_t);
template void BufferObject::uploadData<uint8_t>(uint32_t, const std::vector<uint8_t> &, uint32_t);
template void BufferObject::uploadData<uint32_t>(uint32_t, const std::vector<uint32_t> &, uint32_t);
template std::vector<uint8_t> BufferObject::downloadData<uint8_t>(uint32_t, std::size_t) const;
template std::vector<uint32_t> BufferObject::downloadData<uint32_t>(uint32_t, std::size_t) const;

}

/*
 This file is part of the Rendering library.
 Copyright (C) 2014 Sascha Brandt <myeti@mail.upb.de>

 This library is subject to the terms of the Mozilla Public License, v. 2.0.
 You should have received a copy of the MPL along with this library; see the
 file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef RENDERING_HAS_LIB_OPENCL
#ifndef RENDERING_CL_BUFFERACCESSOR_H_
#define RENDERING_CL_BUFFERACCESSOR_H_

#include "Buffer.h"
#include "../CLUtils.h"

#include <Util/ReferenceCounter.h>
#include <Util/Macros.h>

#include <vector>

namespace Rendering {
namespace CL {
class CommandQueue;

/*! Class for direct access to an OpenCL Buffer.

	\note All buffer altering operations should be made between a call of BufferAccessor::begin() and BufferAccessor::end()
	\note BufferAccessor::end() has to be called before using the buffer again.
*/
class BufferAccessor : public Util::ReferenceCounter<BufferAccessor> {
public:
	BufferAccessor(Buffer* buffer, CommandQueue* queue);
	BufferAccessor(BufferRef buffer, CommandQueueRef queue);
	virtual ~BufferAccessor();

	void begin(ReadWrite_t readWrite = ReadWrite_t::ReadWrite);
	void end();
	inline bool isValid() const { return dataPtr != nullptr; }
	inline void setCursor(size_t offset) { cursor = std::min(offset, size); }
	inline size_t getCursor() const { return cursor; }
	inline size_t getSize() const { return size; }

	template<typename T>
	void write(const T& value);

	template<typename T>
	inline void writeArray(const std::vector<T>& value);

	template<typename T>
	inline T read();

	template<typename T>
	inline std::vector<T> readArray(size_t numValues);

	inline uint8_t* _ptr() const { return dataPtr; }
private:
	BufferRef buffer;
	CommandQueueRef queue;
	uint8_t* dataPtr;
	size_t cursor;
	size_t size;
};

template<typename T>
void BufferAccessor::write(const T& value) {
	THROW_ERROR_IF(!isValid(), "Called write() before begin().");
	THROW_ERROR_IF(cursor+sizeof(T) > size, "Could not write value. End of buffer reached.");
	*(reinterpret_cast<T*>(dataPtr + cursor)) = value;
	cursor += sizeof(T);
}

template<typename T>
inline void BufferAccessor::writeArray(const std::vector<T>& value) {
	THROW_ERROR_IF(!isValid(), "Called writeArray() before begin().");
	THROW_ERROR_IF(cursor+sizeof(T)*value.size() > size, "Could not write array. End of buffer reached.");
	std::copy(value.begin(), value.end(), reinterpret_cast<T*>(dataPtr + cursor));
	cursor += sizeof(T) * value.size();
}

template<typename T>
inline T BufferAccessor::read() {
	T out;
	THROW_ERROR_IF(!isValid(), "Called read() before begin().");
	THROW_ERROR_IF(cursor+sizeof(T) > size, "Could not read value. End of buffer reached.");
	out = *(reinterpret_cast<T*>(dataPtr + cursor));
	cursor += sizeof(T);
	return out;
}

template<typename T>
inline std::vector<T> BufferAccessor::readArray(size_t numValues) {
	THROW_ERROR_IF(!isValid(), "Called readArray() before begin().");
	THROW_ERROR_IF(cursor+sizeof(T)*numValues > size, "Could not read array. End of buffer reached.");

	std::vector<T> out(reinterpret_cast<T*>(dataPtr + cursor), reinterpret_cast<T*>(dataPtr + cursor + sizeof(T) * numValues));
	cursor += sizeof(T) * numValues;
	return out;
}

} /* namespace CL */
} /* namespace Rendering */

#endif /* RENDERING_CL_BUFFERACCESSOR_H_ */
#endif /* RENDERING_HAS_LIB_OPENCL */

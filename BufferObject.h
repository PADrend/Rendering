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
#ifndef RENDERING_BUFFEROBJECT_H
#define RENDERING_BUFFEROBJECT_H

#include "Core/Common.h"

#include <Util/CountedObjectWrapper.h>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace Rendering {
class Device;
using DeviceRef = Util::Reference<Device>;
class BufferStorage;
using BufferStorageRef = Util::Reference<BufferStorage>;
	
//! @defgroup rendering_resources Resources
	
/**
 * Representation of an OpenGL buffer object (BO).
 *
 * @author Benjamin Eikel
 * @date 2012-04-19
 * @ingroup rendering_resources
 */
class BufferObject : public Util::ReferenceCounter<BufferObject> {
public:
	using Ref = Util::Reference<BufferObject>;

	static const uint32_t TARGET_ARRAY_BUFFER;
	static const uint32_t TARGET_ATOMIC_COUNTER_BUFFER;
	static const uint32_t TARGET_COPY_READ_BUFFER;
	static const uint32_t TARGET_COPY_WRITE_BUFFER;
	static const uint32_t TARGET_DISPATCH_INDIRECT_BUFFER;
	static const uint32_t TARGET_DRAW_INDIRECT_BUFFER;
	static const uint32_t TARGET_ELEMENT_ARRAY_BUFFER;
	static const uint32_t TARGET_PIXEL_PACK_BUFFER;
	static const uint32_t TARGET_PIXEL_UNPACK_BUFFER;
	static const uint32_t TARGET_QUERY_BUFFER;
	static const uint32_t TARGET_SHADER_STORAGE_BUFFER;
	static const uint32_t TARGET_TEXTURE_BUFFER;
	static const uint32_t TARGET_TRANSFORM_FEEDBACK_BUFFER;
	static const uint32_t TARGET_UNIFORM_BUFFER;
	
	static const uint32_t USAGE_STREAM_DRAW;
	static const uint32_t USAGE_STREAM_READ;
	static const uint32_t USAGE_STREAM_COPY;
	static const uint32_t USAGE_STATIC_DRAW;
	static const uint32_t USAGE_STATIC_READ;
	static const uint32_t USAGE_STATIC_COPY;
	static const uint32_t USAGE_DYNAMIC_DRAW;
	static const uint32_t USAGE_DYNAMIC_READ;
	static const uint32_t USAGE_DYNAMIC_COPY;
	
	enum class AccessFlag : uint8_t {
		NO_ACCESS = 0,
		READ_ONLY = 1,
		WRITE_ONLY = 2,
		READ_WRITE = 3
	};

	//! Create an invalid buffer object.
	static Ref create(const DeviceRef& device);

	//! Create an invalid buffer object for the given target.
	[[deprecated]]
	BufferObject();

	//! Data of an buffer object should not be copied.
	BufferObject(const BufferObject &) = delete;

	//! Take ownership of the data of the other buffer object.
	BufferObject(BufferObject && other) = default;

	//! Free the data of the buffer object.
	~BufferObject();

	//! Data of an buffer object should not be copied.
	BufferObject & operator=(const BufferObject &) = delete;

	//! Take ownership of the data of the other buffer object.
	BufferObject & operator=(BufferObject && other) = default;

	//! Swap the buffer with another BufferObject
	void swap(BufferObject& other);

	//! Request a new handle from OpenGL for this buffer object.
	[[deprecated]]
	void prepare() {}

	//! Free the handle of this buffer object.
	void destroy();

	//! Bind the buffer object to the given target.
	[[deprecated]]
	void bind(uint32_t bufferTarget) const {}
	[[deprecated]]
	void bind(uint32_t bufferTarget, uint32_t location) const {}

	//! Remove any binding of the given target.
	[[deprecated]]
	void unbind(uint32_t bufferTarget) const {}
	[[deprecated]]
	void unbind(uint32_t bufferTarget, uint32_t location) const {}

	/**
	 * @brief Allocate buffer data
	 * 
	 * Bind the buffer object to the given target,
	 * allocate @a numberOfElements times <tt>sizeof(T)</tt> bytes,
	 * and unbind the buffer object.
	 */
	bool allocate(size_t size, ResourceUsage usage = ResourceUsage::General, MemoryUsage access = MemoryUsage::CpuToGpu, bool persistent=false);

	template<typename T>
	[[deprecated]]
	void allocateData(uint32_t bufferTarget, std::size_t numberOfElements, uint32_t usageHint) {
		allocate(numberOfElements * sizeof(T));
	}

	/**
	 * @brief Copy data to the buffer object
	 * 
	 * Bind the buffer object to the given target,
	 * copy <tt>data.size()</tt> times <tt>sizeof(T)</tt> bytes from the vector to the buffer object,
	 * and unbind the buffer object.
	 */
	void upload(const uint8_t* data, size_t numBytes);

	template<typename T>
	void upload(const std::vector<T>& data) {
		upload(reinterpret_cast<uint8_t*>(data.data()), data.size() * sizeof(T));
	}

	template<typename T>
	[[deprecated]]
	void uploadData(uint32_t bufferTarget, const std::vector<T> & data, uint32_t usageHint) {
		upload(reinterpret_cast<uint8_t*>(data.data()), data.size() * sizeof(T));
	}
	[[deprecated]]
	void uploadData(uint32_t bufferTarget, const uint8_t* data, size_t numBytes, uint32_t usageHint) {
		upload(data, numBytes);
	}
	
	/**
	 * @brief Copy data to the buffer object
	 * 
	 * Bind the buffer object to the given target,
	 * copy <tt>data.size()</tt> times <tt>sizeof(T)</tt> bytes from the vector to the buffer object,
	 * and unbind the buffer object.
	 */
	template<typename T>
	[[deprecated]]
	void uploadSubData(uint32_t bufferTarget, const std::vector<T> & data, size_t offset=0) {
		upload(reinterpret_cast<uint8_t*>(data.data()), data.size() * sizeof(T));
	}
	[[deprecated]]
	void uploadSubData(uint32_t bufferTarget, const uint8_t* data, size_t numBytes, size_t offset=0) {
		upload(data, numBytes);
	}
	
	/**
	 * @brief Retrieve data from the buffer object
	 * 
	 * Bind the buffer object to the given target,
	 * copy @a numberOfElements times <tt>sizeof(T)</tt> bytes from the buffer object to the vector,
	 * and unbind the buffer object.
	 */
	std::vector<uint8_t> download(size_t range, size_t offset=0);

	template<typename T>
	[[deprecated]]
	std::vector<T> downloadData(uint32_t bufferTarget, size_t numberOfElements, size_t offset=0) {
		const T* bufferData = reinterpret_cast<const T*>(map());
		const std::vector<T> result(bufferData + offset, bufferData + offset + numberOfElements);
		unmap();
		return result;
	}

	//! @c true if and only if prepare() was executed at least once without an execution of destroy() afterwards. 
	bool isValid() const {
		return buffer.isNotNull();
	}
	[[deprecated]]
	uint32_t getGLId() const { return 0; }
	
	[[deprecated]]
	void clear(uint32_t bufferTarget, uint32_t internalFormat, uint32_t format, uint32_t type, const uint8_t* data=nullptr);
	[[deprecated]]
	void clear(uint32_t internalFormat, uint32_t format, uint32_t type, const uint8_t* data=nullptr);
	
	void copy(const BufferObject& source, uint32_t sourceOffset, uint32_t targetOffset, uint32_t size);
	
	/** 
	 * Map the buffer object's data store into the client's address space
	 *
	 * @return The mapped data pointer, or <tt>nullptr</tt> if the mapping failed.
	 */
	uint8_t* map();

	[[deprecated]]
	uint8_t* map(uint32_t offset, uint32_t size=0, AccessFlag access=AccessFlag::READ_WRITE) { return map(); }
	
	/** 
	 * Unmaps a previously mapped buffer.
	 */
	void unmap();

	/*!	@name Internal */
	// @{
	const BufferStorageRef& getBuffer() const { return buffer; }
	// @}

private:
	BufferObject(const DeviceRef& device);

	DeviceRef device;
	//! Internal device buffer storage
	BufferStorageRef buffer;
};

typedef BufferObject::Ref CountedBufferObject;

}

#endif /* RENDERING_BUFFEROBJECT_H */

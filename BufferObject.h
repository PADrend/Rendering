/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	Copyright (C) 2014-2019 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_BUFFEROBJECT_H
#define RENDERING_BUFFEROBJECT_H

#include <cstddef>
#include <cstdint>
#include <vector>
#include <Util/CountedObjectWrapper.h>

namespace Rendering {
	
//! @defgroup rendering_resources Resources
	
/**
 * Representation of an OpenGL buffer object (BO).
 *
 * @author Benjamin Eikel
 * @date 2012-04-19
 * @ingroup rendering_resources
 */
class BufferObject {
	public:		
		RENDERINGAPI static const uint32_t TARGET_ARRAY_BUFFER;
		RENDERINGAPI static const uint32_t TARGET_ATOMIC_COUNTER_BUFFER;
		RENDERINGAPI static const uint32_t TARGET_COPY_READ_BUFFER;
		RENDERINGAPI static const uint32_t TARGET_COPY_WRITE_BUFFER;
		RENDERINGAPI static const uint32_t TARGET_DISPATCH_INDIRECT_BUFFER;
		RENDERINGAPI static const uint32_t TARGET_DRAW_INDIRECT_BUFFER;
		RENDERINGAPI static const uint32_t TARGET_ELEMENT_ARRAY_BUFFER;
		RENDERINGAPI static const uint32_t TARGET_PIXEL_PACK_BUFFER;
		RENDERINGAPI static const uint32_t TARGET_PIXEL_UNPACK_BUFFER;
		RENDERINGAPI static const uint32_t TARGET_QUERY_BUFFER;
		RENDERINGAPI static const uint32_t TARGET_SHADER_STORAGE_BUFFER;
		RENDERINGAPI static const uint32_t TARGET_TEXTURE_BUFFER;
		RENDERINGAPI static const uint32_t TARGET_TRANSFORM_FEEDBACK_BUFFER;
		RENDERINGAPI static const uint32_t TARGET_UNIFORM_BUFFER;
		
		RENDERINGAPI static const uint32_t USAGE_STREAM_DRAW;
		RENDERINGAPI static const uint32_t USAGE_STREAM_READ;
		RENDERINGAPI static const uint32_t USAGE_STREAM_COPY;
		RENDERINGAPI static const uint32_t USAGE_STATIC_DRAW;
		RENDERINGAPI static const uint32_t USAGE_STATIC_READ;
		RENDERINGAPI static const uint32_t USAGE_STATIC_COPY;
		RENDERINGAPI static const uint32_t USAGE_DYNAMIC_DRAW;
		RENDERINGAPI static const uint32_t USAGE_DYNAMIC_READ;
		RENDERINGAPI static const uint32_t USAGE_DYNAMIC_COPY;
		
		enum class AccessFlag : uint8_t {
			NO_ACCESS = 0,
			READ_ONLY = 1,
			WRITE_ONLY = 2,
			READ_WRITE = 3
		};
	
	private:
		//! OpenGL handle for this buffer object.
		uint32_t bufferId;

	public:
		//! Create an invalid buffer object for the given target.
		RENDERINGAPI BufferObject();

		//! Data of an buffer object should not be copied.
		BufferObject(const BufferObject &) = delete;

		//! Take ownership of the data of the other buffer object.
		RENDERINGAPI BufferObject(BufferObject && other);

		//! Free the data of the buffer object.
		RENDERINGAPI ~BufferObject();

		//! Data of an buffer object should not be copied.
		BufferObject & operator=(const BufferObject &) = delete;

		//! Take ownership of the data of the other buffer object.
		RENDERINGAPI BufferObject & operator=(BufferObject && other);

		//! Swap the gl buffer with another BufferObject
		RENDERINGAPI void swap(BufferObject & other);

		//! Request a new handle from OpenGL for this buffer object.
		RENDERINGAPI void prepare();

		//! Free the handle of this buffer object.
		RENDERINGAPI void destroy();

		//! Bind the buffer object to the given target.
		RENDERINGAPI void bind(uint32_t bufferTarget) const;
		RENDERINGAPI void bind(uint32_t bufferTarget, uint32_t location) const;

		//! Remove any binding of the given target.
		RENDERINGAPI void unbind(uint32_t bufferTarget) const;
		RENDERINGAPI void unbind(uint32_t bufferTarget, uint32_t location) const;

		/**
		 * @brief Allocate buffer data
		 * 
		 * Bind the buffer object to the given target,
		 * allocate @a numberOfElements times <tt>sizeof(T)</tt> bytes,
		 * and unbind the buffer object.
		 */
		template<typename T>
		void allocateData(uint32_t bufferTarget, std::size_t numberOfElements, uint32_t usageHint) {
			uploadData(bufferTarget, nullptr, numberOfElements * sizeof(T), usageHint);
		}

		/**
		 * @brief Copy data to the buffer object
		 * 
		 * Bind the buffer object to the given target,
		 * copy <tt>data.size()</tt> times <tt>sizeof(T)</tt> bytes from the vector to the buffer object,
		 * and unbind the buffer object.
		 */
		template<typename T>
		void uploadData(uint32_t bufferTarget, const std::vector<T> & data, uint32_t usageHint) {
			uploadData(bufferTarget, reinterpret_cast<const uint8_t*>(data.data()),data.size() * sizeof(T),usageHint);
		}
		RENDERINGAPI void uploadData(uint32_t bufferTarget, const uint8_t* data, size_t numBytes, uint32_t usageHint);
		
		/**
		 * @brief Copy data to the buffer object
		 * 
		 * Bind the buffer object to the given target,
		 * copy <tt>data.size()</tt> times <tt>sizeof(T)</tt> bytes from the vector to the buffer object,
		 * and unbind the buffer object.
		 */
		template<typename T>
		void uploadSubData(uint32_t bufferTarget, const std::vector<T> & data, size_t offset=0) {
			uploadSubData(bufferTarget, reinterpret_cast<const uint8_t*>(data.data()),data.size() * sizeof(T),offset);
		}
		RENDERINGAPI void uploadSubData(uint32_t bufferTarget, const uint8_t* data, size_t numBytes, size_t offset=0);
		
		/**
		 * @brief Retrieve data from the buffer object
		 * 
		 * Bind the buffer object to the given target,
		 * copy @a numberOfElements times <tt>sizeof(T)</tt> bytes from the buffer object to the vector,
		 * and unbind the buffer object.
		 */
		RENDERINGAPI void downloadData(uint32_t bufferTarget, size_t numBytes, uint8_t* targetPtr, size_t offset=0) const;
		template<typename T>
		std::vector<T> downloadData(uint32_t bufferTarget, size_t numberOfElements, size_t offset=0) const {
			std::vector<T> result(numberOfElements);
			downloadData(bufferTarget, numberOfElements * sizeof(T), reinterpret_cast<uint8_t*>(result.data()), offset);
			return result;
		}
		
		//! @c true if and only if prepare() was executed at least once without an execution of destroy() afterwards. 
		bool isValid() const {
			return bufferId != 0;
		}
		uint32_t getGLId()const{	return bufferId;	}
		
		RENDERINGAPI void clear(uint32_t bufferTarget, uint32_t internalFormat, uint32_t format, uint32_t type, const uint8_t* data=nullptr);
		RENDERINGAPI void clear(uint32_t internalFormat, uint32_t format, uint32_t type, const uint8_t* data=nullptr);
		RENDERINGAPI void clear();
    
    RENDERINGAPI void copy(const BufferObject& source, uint32_t sourceOffset, uint32_t targetOffset, uint32_t size);
		
		/** 
		 * Map all or part of a buffer object's data store into the client's address space
		 * @note The buffer has to be unmapped before using it for rendering.
		 * 
		 * @param offset The offset into the buffer in bytes.
		 * @param size The size of the buffer range in bytes to map. When <tt>size</tt> is 0, the entire buffer is mapped.
		 * @param allowWrite Indicates that the returned pointer may be used to modify buffer object data.
		 * @param allowRead Indicates that the returned pointer may be used to read buffer object data.
		 * @return The mapped data pointer, or <tt>nullptr</tt> if the mapping failed.
		 */
		RENDERINGAPI uint8_t* map(uint32_t offset=0, uint32_t size=0, AccessFlag access=AccessFlag::READ_WRITE);
		
		/** 
		 * Unmaps a previously mapped buffer.
		 */
		RENDERINGAPI void unmap();

		//! (internal) sets the glId of the buffer. Used for creating buffers from existing gl buffers. 
		RENDERINGAPI void _setGLId(uint32_t glId);
};

typedef Util::CountedObjectWrapper<BufferObject> CountedBufferObject;

}

#endif /* RENDERING_BUFFEROBJECT_H */

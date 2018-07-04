/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	Copyright (C) 2014-2018 Sascha Brandt <sascha@brandt.graphics>
	
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

/**
 * Representation of an OpenGL buffer object (BO).
 *
 * @author Benjamin Eikel
 * @date 2012-04-19
 */
class BufferObject {
	public:
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
		
		static const uint32_t USAGE_STREAM_DRAW __attribute((deprecated));
		static const uint32_t USAGE_STREAM_READ __attribute((deprecated));
		static const uint32_t USAGE_STREAM_COPY __attribute((deprecated));
		static const uint32_t USAGE_STATIC_DRAW __attribute((deprecated));
		static const uint32_t USAGE_STATIC_READ __attribute((deprecated));
		static const uint32_t USAGE_STATIC_COPY __attribute((deprecated));
		static const uint32_t USAGE_DYNAMIC_DRAW __attribute((deprecated));
		static const uint32_t USAGE_DYNAMIC_READ __attribute((deprecated));
		static const uint32_t USAGE_DYNAMIC_COPY __attribute((deprecated));
	
		static const uint32_t FLAG_DYNAMIC_STORAGE;
		static const uint32_t FLAG_MAP_READ;
		static const uint32_t FLAG_MAP_WRITE;
		static const uint32_t FLAG_MAP_PERSISTENT;
		static const uint32_t FLAG_MAP_COHERENT;
		static const uint32_t FLAG_CLIENT_STORAGE;
		
		static const uint32_t FLAG_MAP_INVALIDATE_RANGE;
		static const uint32_t FLAG_MAP_INVALIDATE_BUFFFER;
		static const uint32_t FLAG_MAP_FLUSH_EXPLICIT;
		static const uint32_t FLAG_MAP_UNSYNCHRONIZED;
		
		static const uint32_t FLAGS_STATIC;
		static const uint32_t FLAGS_DYNAMIC;
		static const uint32_t FLAGS_PERSISTENT;
		static const uint32_t FLAGS_STREAM;
	private:		
		//! OpenGL handle for this buffer object.
		uint32_t bufferId = 0;
		uint32_t flags = 0;
		size_t size = 0;
		uint8_t* ptr = nullptr;

	public:
		//! Create an invalid buffer object for the given target.
		BufferObject() {};

		//! Data of an buffer object should not be copied.
		BufferObject(const BufferObject &) = delete;

		//! Take ownership of the data of the other buffer object.
		BufferObject(BufferObject && other);

		//! Free the data of the buffer object.
		~BufferObject();

		//! Data of an buffer object should not be copied.
		BufferObject & operator=(const BufferObject &) = delete;

		//! Take ownership of the data of the other buffer object.
		BufferObject & operator=(BufferObject && other);

		//! Swap the gl buffer with another BufferObject
		void swap(BufferObject & other);

		//! Request a new handle from OpenGL for this buffer object.
		void prepare();

		//! Free the handle of this buffer object.
		void destroy();

		//! Bind the buffer object to the given target.
		void bind(uint32_t bufferTarget) const;
		void bind(uint32_t bufferTarget, uint32_t location) const;
		void bindRange(uint32_t bufferTarget, uint32_t location, size_t offset, size_t size) const;

		//! Remove any binding of the given target.
		void unbind(uint32_t bufferTarget) const;
		void unbind(uint32_t bufferTarget, uint32_t location) const;

		/**
		 * @brief Allocate buffer data
		 * 
		 * Bind the buffer object to the given target,
		 * allocate @a numberOfElements times <tt>sizeof(T)</tt> bytes,
		 * and unbind the buffer object.
		 */
		template<typename T>
		void __attribute((deprecated)) allocateData(uint32_t bufferTarget, size_t numberOfElements, uint32_t flags) {
			allocate(numberOfElements, flags);
		}

		/**
		 * @brief Copy data to the buffer object
		 * 
		 * Bind the buffer object to the given target,
		 * copy <tt>data.size()</tt> times <tt>sizeof(T)</tt> bytes from the vector to the buffer object,
		 * and unbind the buffer object.
		 */
	 	void allocate(size_t numBytes, uint32_t flags, const uint8_t* initialData=nullptr);
		template<typename T>
		void allocate(const std::vector<T>& initialData, uint32_t flags) {
			allocate(initialData.size() * sizeof(T), flags, reinterpret_cast<const uint8_t*>(initialData.data()));
		}
		template<typename T>
		void __attribute((deprecated)) uploadData(uint32_t bufferTarget, const std::vector<T> & data, uint32_t flags) {
			allocate<T>(data, flags);
		}
		void __attribute((deprecated)) uploadData(uint32_t bufferTarget, const uint8_t* data, size_t numBytes, uint32_t flags) {
			allocate(numBytes, flags, data);
		}
		
		/**
		 * @brief Copy data to the buffer object
		 * 
		 * Bind the buffer object to the given target,
		 * copy <tt>data.size()</tt> times <tt>sizeof(T)</tt> bytes from the vector to the buffer object,
		 * and unbind the buffer object.
		 */
		void upload(const uint8_t* data, size_t numBytes, size_t offset=0);
		template<typename T>
		void upload(const std::vector<T> & data, size_t offset=0) {
			upload(reinterpret_cast<const uint8_t*>(data.data()), data.size() * sizeof(T), offset);
		}
		template<typename T>
		void __attribute((deprecated)) uploadSubData(uint32_t bufferTarget, const std::vector<T> & data, size_t offset=0) {
			uploadSubData<T>(data, offset);
		}
		void __attribute((deprecated)) uploadSubData(uint32_t bufferTarget, const uint8_t* data, size_t numBytes, size_t offset=0) {
			upload(data, numBytes, offset);
		}
		
		/**
		 * @brief Retrieve data from the buffer object
		 * 
		 * Bind the buffer object to the given target,
		 * copy @a numberOfElements times <tt>sizeof(T)</tt> bytes from the buffer object to the vector,
		 * and unbind the buffer object.
		 */
		void download(uint8_t* targetPtr, size_t numBytes, size_t offset=0) const;
		template<typename T>
		std::vector<T> download(size_t numberOfElements, size_t first=0) const {
			std::vector<T> result(numberOfElements);
			download(reinterpret_cast<uint8_t*>(result.data()), numberOfElements * sizeof(T), first * sizeof(T));
			return result;
		}
		template<typename T>
		std::vector<T> __attribute((deprecated)) downloadData(uint32_t bufferTarget, size_t numberOfElements) const {
			return download<T>(numberOfElements);
		}

		uint8_t* map(size_t offset=0, size_t range=0, uint32_t mapFlags=0);
		const uint8_t* map(size_t offset=0) const;
		void unmap();
		void flush(size_t offset=0, size_t range=0);
		
		void clear(uint32_t internalFormat, uint32_t format, uint32_t type, const uint8_t* data=nullptr);
		void __attribute((deprecated)) clear(uint32_t bufferTarget, uint32_t internalFormat, uint32_t format, uint32_t type, const uint8_t* data=nullptr) {
			clear(internalFormat, format, type, data);
		}
		
		//! @c true if and only if prepare() was executed at least once without an execution of destroy() afterwards. 
		bool isValid() const { return bufferId != 0; }
		uint32_t getGLId() const { return bufferId; }
		size_t getSize() const { return size; }
		uint32_t getFlags() const { return flags; }
};

typedef Util::CountedObjectWrapper<BufferObject> CountedBufferObject;

}

#endif /* RENDERING_BUFFEROBJECT_H */

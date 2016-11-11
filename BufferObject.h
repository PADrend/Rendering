/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
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
	private:
		//! OpenGL handle for this buffer object.
		uint32_t bufferId;

	public:
		//! Create an invalid buffer object for the given target.
		BufferObject();

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
		void allocateData(uint32_t bufferTarget, std::size_t numberOfElements, uint32_t usageHint);

		/**
		 * @brief Copy data to the buffer object
		 * 
		 * Bind the buffer object to the given target,
		 * copy <tt>data.size()</tt> times <tt>sizeof(T)</tt> bytes from the vector to the buffer object,
		 * and unbind the buffer object.
		 */
		template<typename T>
		void uploadData(uint32_t bufferTarget, const std::vector<T> & data, uint32_t usageHint);
		void uploadData(uint32_t bufferTarget, const uint8_t* data, size_t numBytes, uint32_t usageHint);

		/**
		 * @brief Retrieve data from the buffer object
		 * 
		 * Bind the buffer object to the given target,
		 * copy @a numberOfElements times <tt>sizeof(T)</tt> bytes from the buffer object to the vector,
		 * and unbind the buffer object.
		 */
		template<typename T>
		std::vector<T> downloadData(uint32_t bufferTarget, std::size_t numberOfElements) const;

		//! @c true if and only if prepare() was executed at least once without an execution of destroy() afterwards. 
		bool isValid() const {
			return bufferId != 0;
		}
		uint32_t getGLId()const{	return bufferId;	}
		
		void clear(uint32_t bufferTarget, uint32_t internalFormat, uint32_t format, uint32_t type, const uint8_t* data=nullptr);
		void clear(uint32_t internalFormat, uint32_t format, uint32_t type, const uint8_t* data=nullptr);
};

typedef Util::CountedObjectWrapper<BufferObject> CountedBufferObject;

}

#endif /* RENDERING_BUFFEROBJECT_H */

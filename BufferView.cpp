/*
	This file is part of the Rendering library.
	Copyright (C) 2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "BufferView.h"
#include <Util/Macros.h>

#include <cstdlib>

namespace Rendering {
  
void BufferView::relocate(CountedBufferObject* buffer_, size_t offset_) {
  release();
  buffer = buffer_;
  offset = offset_;
}

void BufferView::allocate(uint32_t count, bool createLocalCopy) {
  release();
  if(buffer.isNull() || !buffer->get().isValid())
    throw std::runtime_error("BufferView::allocate: invalid buffer");
  
  elementCount = count;
  isMappedPtr = false;
  auto flags = buffer->get().getFlags();
  if(flags & BufferObject::FLAG_MAP_PERSISTENT) {
    dataPtr = buffer->get().map(offset, dataSize());
    isMappedPtr = true;
  } else if(createLocalCopy) {
    dataPtr = reinterpret_cast<uint8_t*>(std::malloc(dataSize()));
  }
}

void BufferView::release() {
  if(!isMappedPtr && dataPtr)
    std::free(dataPtr);
  elementCount = 0;
  dataPtr = nullptr;
}

void BufferView::flush() {
  if(!dataPtr) return;
  if(isMappedPtr) {
    buffer->get().flush(offset, dataSize());
  } else {
    buffer->get().upload(dataPtr, dataSize(), offset);
  }
}

void BufferView::upload(const uint8_t* ptr, size_t size) {
  buffer->get().upload(ptr, size, offset);
}

void BufferView::bind(uint32_t target, uint32_t location) {
  buffer->get().bindRange(target, location, offset, dataSize());
}

void BufferView::unbind(uint32_t target, uint32_t location) {
  buffer->get().unbind(target, location);
}

} /* Rendering */

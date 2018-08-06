/*
	This file is part of the Rendering library.
	Copyright (C) 2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "BufferView.h"
#include <Util/Macros.h>
#include "../GLHeader.h"

#include <cstdlib>
#include <iostream>

namespace Rendering {

BufferView::BufferView(BufferObject* _buffer, size_t offset, uint32_t eltSize, uint32_t count) : 
          buffer(_buffer), offset(offset), elementSize(std::max(eltSize, 1U)), elementCount(count) {
  if(_buffer && count == 0) 
    elementCount = (buffer->getSize() - offset) / elementSize;
}

void BufferView::swap(BufferView& other) {
	if(this == &other)
		return;
  
  std::swap(buffer, other.buffer);
  std::swap(offset, other.offset);
  std::swap(elementSize, other.elementSize);
  std::swap(elementCount, other.elementCount);
}

bool BufferView::isValid() const {
  return     buffer.isNotNull() 
          && buffer->isValid()
          && elementCount > 0
          && elementSize > 0
          && buffer->getSize() >= offset+elementCount*elementSize;
}

void BufferView::bind(uint32_t target, uint32_t location) {
  if(isValid()) {
    buffer->bind(target, location, getOffset(), getSize());
  } else {
    WARN("BufferView::bind: invalid buffer or data size.");
  }
}

void BufferView::allocateBuffer(uint32_t flags, const uint8_t* initialData) {
  if(buffer.isNull())
    buffer = new BufferObject;
  if(buffer->isValid())
    buffer->destroy();
  buffer->allocate(elementCount*elementSize, flags, initialData);
}

void BufferView::setValues(uint32_t index, uint32_t count, const uint8_t* data) {
  if(!isValid()) {
    WARN("BufferView::setValues: invalid buffer or data size.");
    return;
  } else if(index+count > elementCount) {
    WARN("BufferView::setValues: index out of range.");
    return;
  }
  buffer->upload(data, elementSize*count, getOffset() + elementSize*index);
}

void BufferView::getValues(uint32_t index, uint32_t count, uint8_t* targetPtr) const {
  if(!isValid()) {
    WARN("BufferView::getValues: invalid buffer or data size.");
    return;
  } else if(index+count > elementCount) {
    WARN("BufferView::getValues: index out of range.");
    return;
  }
  buffer->download(targetPtr, elementSize*count, getOffset() + elementSize*index);
}

} /* Rendering */

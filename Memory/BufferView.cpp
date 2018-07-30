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
          
void BufferView::setMultiBuffered(uint32_t count) {
  lock = BufferLockManager(); // reset all locks
  multiBufferCount = count;
  multiBufferHead = 0;
}

bool BufferView::isValid() const {
  return     buffer.isNotNull() 
          && buffer->isValid()
          && elementCount > 0
          && elementSize > 0
          && buffer->getSize() >= offset+elementCount*elementSize*multiBufferCount;
}

void BufferView::bind(uint32_t target, uint32_t location) {
  if(isValid()) {
    buffer->bind(target, location, getOffset(), getSize());
  } else {
    WARN("BufferView::bind: invalid buffer or data size.");
  }
}

void BufferView::allocateBuffer(uint32_t flags) {
  if(buffer.isNull())
    buffer = new BufferObject;
  if(buffer->isValid())
    buffer->destroy();
  buffer->allocate(elementCount*elementSize*multiBufferCount, flags);
}

void BufferView::setValues(uint32_t index, uint32_t count, const uint8_t* data) {
  if(!isValid()) {
    WARN("BufferView::setValues: invalid buffer or data size.");
    return;
  } else if(index+count > elementCount) {
    WARN("BufferView::setValues: index out of range.");
    return;
  }
  lock.waitForLockedRange(multiBufferHead*elementCount + index, count);
  buffer->upload(data, elementSize*count, getOffset() + elementSize*index);
}

void BufferView::getValues(uint32_t index, uint32_t count, uint8_t* targetPtr) {
  if(!isValid()) {
    WARN("BufferView::getValues: invalid buffer or data size.");
    return;
  } else if(index+count > elementCount) {
    WARN("BufferView::getValues: index out of range.");
    return;
  }
  lock.waitForLockedRange(multiBufferHead*elementCount + index, count);
  buffer->download(targetPtr, elementSize*count, getOffset() + elementSize*index);
}

void BufferView::swap() {
  lock.lockRange(multiBufferHead*elementCount, elementCount);
  multiBufferHead = (multiBufferHead+1) % multiBufferCount;
}

} /* Rendering */

/*
	This file is part of the Rendering library.
	Copyright (C) 2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "StreamBufferView.h"
#include <Util/Macros.h>
#include "../GLHeader.h"

#include <cstdlib>
#include <iostream>
#include <algorithm>

namespace Rendering {

StreamBufferView::StreamBufferView(uint32_t multiBufferCount, BufferObject* _buffer, size_t offset, uint32_t eltSize, uint32_t count) : 
          BufferView(_buffer, offset, eltSize, count), baseOffset(offset), multiBufferCount(multiBufferCount) {
}

void StreamBufferView::allocateBuffer(uint32_t flags, const uint8_t* initialData) {
  auto buffer = getBuffer();
  if(!buffer) {
    buffer = new BufferObject;
    setBuffer(buffer);
  }
  if(buffer->isValid())
    buffer->destroy();
  setOffset(0);
  multiBufferHead = 0;
  head = 0;
  buffer->allocate(getSize()*multiBufferCount, flags, initialData);
}

void StreamBufferView::setValues(uint32_t index, uint32_t count, const uint8_t* data) {
  lock.waitForLockedRange(multiBufferHead*getElementCount() + index, count);
  BufferView::setValues(index, count, data);
}

void StreamBufferView::getValues(uint32_t index, uint32_t count, uint8_t* targetPtr) const {
  lock.waitForLockedRange(multiBufferHead*getElementCount() + index, count);
  BufferView::getValues(index, count, targetPtr);
}

void StreamBufferView::swap() {
  lock.lockRange(multiBufferHead*getElementCount(), getElementCount());
  multiBufferHead = (multiBufferHead+1) % multiBufferCount;
  setOffset(baseOffset + multiBufferHead*getSize());
  head = 0;
}

} /* Rendering */

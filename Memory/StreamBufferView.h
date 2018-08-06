/*
	This file is part of the Rendering library.
	Copyright (C) 2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_STREAMBUFFERVIEW_H_
#define RENDERING_STREAMBUFFERVIEW_H_

#include "BufferView.h"
#include "BufferLock.h"

#include <vector>

namespace Rendering {

class StreamBufferView : public BufferView {
private:
  size_t baseOffset = 0;
  uint32_t multiBufferCount = 1;
  uint32_t multiBufferHead = 0;
  uint32_t head = 0;
  mutable BufferLockManager lock;
public:
  StreamBufferView(uint32_t multiBufferCount=1, BufferObject* buffer=nullptr, size_t offset=0, uint32_t eltSize=0, uint32_t count=0);
  StreamBufferView(const StreamBufferView& other) = default;
  StreamBufferView(StreamBufferView&& other) = default;
  StreamBufferView& operator=(const StreamBufferView& other) = default;
  
  void setBaseOffset(size_t off) { baseOffset = off; }
  void setMultiBufferCount(uint32_t count) {
    multiBufferCount = count;
    multiBufferHead = 0;
    head = 0;
    lock = BufferLockManager(); // reset all locks
  };
  
  virtual void allocateBuffer(uint32_t flags, const uint8_t* initialData=nullptr) override;
  
  virtual void setValues(uint32_t index, uint32_t count, const uint8_t* data) override;  
  virtual void getValues(uint32_t index, uint32_t count, uint8_t* targetPtr) const override;
  
  void swap();
};

} /* Rendering */

#endif /* end of include guard: RENDERING_STREAMBUFFERVIEW_H_ */

/*
	This file is part of the Rendering library.
	Copyright (C) 2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_BUFFERVIEW_H_
#define RENDERING_BUFFERVIEW_H_

#include "BufferObject.h"
#include "BufferLock.h"
#include <Util/References.h>

#include <vector>

namespace Rendering {

class BufferView : public Util::ReferenceCounter<BufferView>  {
private:
  Util::Reference<BufferObject> buffer;
  size_t offset = 0;
  uint32_t elementSize = 0;
  uint32_t elementCount = 0;
  uint32_t multiBufferCount = 1;
  uint32_t multiBufferHead = 0;
  BufferLockManager lock;
public:
  BufferView(BufferObject* buffer=nullptr, size_t offset=0, uint32_t eltSize=0, uint32_t count=0);

  void setBuffer(BufferObject* bo) { buffer = bo; }
  void setElementCount(uint32_t count) { elementCount = count; }
  void setElementSize(uint32_t size) { elementSize = size; }
  void setMultiBuffered(uint32_t count);

  inline BufferObject* getBuffer() const { return buffer.get(); }
  inline uint32_t getElementCount() const { return elementCount; }
  inline uint32_t getElementSize() const { return elementSize; }
  inline size_t getSize() const { return elementSize * elementCount; }
  inline size_t getBaseOffset() const { return offset; }
  inline size_t getOffset() const { return offset + getSize()*multiBufferHead; }
  bool isValid() const;
  
  void bind(uint32_t target, uint32_t location);
  void allocateBuffer(uint32_t flags);
  
  void setValues(uint32_t index, uint32_t count, const uint8_t* data);
  template<typename T>
  void setValues(uint32_t index, uint32_t count, const T& value) {
    setValue(index, reinterpret_cast<const uint8_t*>(&value));
  }
  
  void setValue(uint32_t index, const uint8_t* data) {
    setValues(index, 1, data);
  };
  template<typename T>
  void setValue(uint32_t index, const T& value) {
    setValues(index, 1, reinterpret_cast<const uint8_t*>(&value));
  }
  
  void getValues(uint32_t index, uint32_t count, uint8_t* targetPtr);
  void getValue(uint32_t index, uint8_t* targetPtr) {
    getValues(index, 1, targetPtr);
  }
  template<typename T>
  T getValue(uint32_t index) {
    T value;
    getValue(index, reinterpret_cast<uint8_t*>(&value));
    return value;
  }
  template<typename T>
  std::vector<T> getValues(uint32_t index, uint32_t count) {
    std::vector<T> values(count);
    getValues(index, count, reinterpret_cast<uint8_t*>(values.data()));
    return values;
  }
  
  // swaps multi-buffered buffer range
  void swap();
};

} /* Rendering */

#endif /* end of include guard: RENDERING_BUFFERVIEW_H_ */

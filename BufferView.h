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
#include <Util/References.h>

#include <vector>

namespace Rendering {

class BufferView {
private:
  Util::Reference<CountedBufferObject> buffer;
  size_t offset = 0;
  uint32_t elementCount = 0;
  uint32_t elementSize = 0;
  uint8_t* dataPtr = nullptr;
  bool isMappedPtr = false;
public:
  BufferView(uint32_t eltSize=0) : elementSize(eltSize) {}
  BufferView(CountedBufferObject* buffer, size_t offset, uint32_t eltSize) : 
                buffer(buffer), offset(offset), elementSize(eltSize) {}
  BufferView(CountedBufferObject* buffer, size_t offset, uint32_t eltSize, uint32_t count) : 
                buffer(buffer), offset(offset), elementSize(eltSize) { allocate(count); }
                
  BufferView(const BufferView & other) = delete;
  BufferView(BufferView &&) = default;

  ~BufferView() { release(); };

  BufferView & operator=(const BufferView &) = delete;
  BufferView & operator=(BufferView &&) = default;
  
  void relocate(CountedBufferObject* buffer, size_t offset);
  void allocate(uint32_t count, bool createLocalCopy=false);
  void release();
  bool hasLocalData() const { return dataPtr; }
  void flush();
  void upload(const uint8_t* ptr, size_t size);
  
  uint32_t getElementCount() const { return elementCount; }
  
  const uint8_t * data() const { return dataPtr; }
  uint8_t * data() { return dataPtr; }
  size_t dataSize() const { return elementCount * elementSize; }
  const uint8_t * operator[](uint32_t index) const { return dataPtr + index*elementSize; }
  uint8_t * operator[](uint32_t index) { return dataPtr + index*elementSize; }
  
  void bind(uint32_t target, uint32_t location);
  void unbind(uint32_t target, uint32_t location);
protected:
  void setElementSize(uint32_t size) {
    release();
    elementSize = size;
  }
};

template<typename T>
class StructuredBufferView : public BufferView {
public:
  using Type_t = T;
  StructuredBufferView() : BufferView(sizeof(Type_t)) {}
  StructuredBufferView(CountedBufferObject* buffer, size_t offset) : BufferView(buffer, offset, sizeof(Type_t)) {}
  StructuredBufferView(CountedBufferObject* buffer, size_t offset, uint32_t count) : BufferView(buffer, offset, sizeof(Type_t), count) {}
  
  const Type_t & operator[](uint32_t index) const { return *reinterpret_cast<const Type_t*>(BufferView::operator[](index)); }
  Type_t & operator[](uint32_t index) { return *reinterpret_cast<Type_t*>(BufferView::operator[](index)); }
  
  void upload(const std::vector<Type_t>& values) {
    BufferView::upload(reinterpret_cast<const uint8_t*>(values.data()), values.size() * sizeof(Type_t));
  };
  void upload(const Type_t* values) {
    BufferView::upload(reinterpret_cast<const uint8_t*>(values), dataSize());
  };
};

template<typename T>
class ValueBufferView : public BufferView {
public:
  using Type_t = T;
  ValueBufferView() : BufferView(sizeof(Type_t)) {}
  ValueBufferView(CountedBufferObject* buffer, size_t offset) : BufferView(buffer, offset, sizeof(Type_t)) {}
  ValueBufferView(CountedBufferObject* buffer, size_t offset, uint32_t count) : BufferView(buffer, offset, sizeof(Type_t), count) {}
  
  void allocate(bool createLocalCopy=false) { BufferView::allocate(1, createLocalCopy); }
  
  const Type_t& getValue() const { return *reinterpret_cast<const Type_t*>(data()); }
  void setValue(const Type_t& value) { return *reinterpret_cast<Type_t*>(data()) = value; }
  
  void upload(const Type_t& value) {
    BufferView::upload(reinterpret_cast<const uint8_t*>(&value), sizeof(Type_t));
  };
};

} /* Rendering */

#endif /* end of include guard: RENDERING_BUFFERVIEW_H_ */

/*
	This file is part of the Rendering library.
	Copyright (C) 2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "ParameterCache.h"

#include "../GLHeader.h"

#include <Util/Macros.h>

#include <limits>
#include <iostream>

namespace Rendering {
  
const uint32_t ParameterCache::INVALID_INDEX = std::numeric_limits<uint32_t>::max();
  
void ParameterCache::createCache(const Util::StringIdentifier& id, uint32_t elementSize, uint32_t maxElementCount, uint32_t usageFlags, uint32_t multiBufferCount) {
  auto it = caches.find(id);
  if(it == caches.end()) {
    CacheEntry entry(id, elementSize, maxElementCount);
    entry.multiBufferCount = std::max(1U, multiBufferCount);
    entry.buffer.allocate(elementSize * maxElementCount * multiBufferCount, usageFlags);
    caches.emplace(id.getValue(), std::move(entry));
  } else if(it->second.buffer.getSize() != elementSize*maxElementCount || it->second.buffer.getFlags() != usageFlags) {
    WARN("A cache named '" + id.toString() + "' already exists, but with different size or usage flags.");
  }
}

//-------

void ParameterCache::deleteCache(const Util::StringIdentifier& id) {
  caches.erase(id);
}

//-------

void ParameterCache::resizeCache(const Util::StringIdentifier& id, uint32_t elementCount) {
  auto& cache = caches.at(id);
  if(cache.maxElementCount == elementCount)
    return;
  BufferObject newBuffer;
  newBuffer.allocate(elementCount*cache.elementSize, cache.buffer.getFlags());
  cache.buffer.copy(newBuffer, std::min(cache.maxElementCount, elementCount)*cache.elementSize);
  cache.buffer.swap(newBuffer);
  cache.maxElementCount = elementCount;
}

//-------

bool ParameterCache::isCache(const Util::StringIdentifier& id) {
  return caches.count(id) > 0;
}

//-------

void ParameterCache::bind(const Util::StringIdentifier& id, uint32_t location, uint32_t target, bool force) {
  auto& cache = caches.at(id);
  if(!force && cache.lastBinding == std::make_pair(location, target))
    return;  
  if(cache.multiBufferCount > 1) {
    size_t size = cache.maxElementCount*cache.elementSize;
    cache.buffer.bindRange(target, location, size * cache.multiBufferHead, size);
  } else {
    cache.buffer.bind(target, location);
  }
  cache.lastBinding = std::make_pair(location, target);
}

//-------

void ParameterCache::setParameter(const Util::StringIdentifier& id, uint32_t index, const uint8_t* data) {
  auto& cache = caches.at(id);
  if(index >= cache.maxElementCount) {
    WARN("ParameterCache::setParameter: index out of range.");
    return;
  }
  uint32_t multiBufferOffset = cache.maxElementCount*cache.multiBufferHead;
	cache.lock.waitForLockedRange(multiBufferOffset + index, 1);
  cache.buffer.upload(data, cache.elementSize, index * cache.elementSize);
}

//-------

uint32_t ParameterCache::addParameter(const Util::StringIdentifier& id, const uint8_t* data) {
  auto& cache = caches.at(id);
  if(cache.head >= cache.maxElementCount)
    return INVALID_INDEX;
  uint32_t index = cache.head++;
  uint32_t multiBufferOffset = cache.maxElementCount*cache.multiBufferHead;
	cache.lock.waitForLockedRange(multiBufferOffset + index, 1);
  cache.buffer.upload(data, cache.elementSize, (multiBufferOffset + index) * cache.elementSize);
  return index;
}

//-------

void ParameterCache::swap(const Util::StringIdentifier& id) {
  auto& cache = caches.at(id);
  cache.lock.lockRange(cache.multiBufferHead*cache.maxElementCount, cache.maxElementCount);
  cache.head = 0;
  cache.multiBufferHead = (cache.multiBufferHead+1)%cache.multiBufferCount;
  cache.lastBinding = {0, 0};
}

} /* Rendering */


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

namespace Rendering {
  
const uint32_t ParameterCache::INVALID_INDEX = std::numeric_limits<uint32_t>::max();
  
void ParameterCache::createCache(const Util::StringIdentifier& id, uint32_t elementSize, uint32_t maxElementCount, uint32_t usageFlags) {
  auto it = caches.find(id);
  if(it == caches.end()) {
    CacheEntry entry{id, BufferObject{}, elementSize, maxElementCount, 0};
    entry.buffer.allocate(elementSize * maxElementCount, usageFlags);
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

void ParameterCache::resetCache(const Util::StringIdentifier& id) {
  auto& cache = caches.at(id);
  cache.head = 0;
  // TODO: synchronize buffer
}
//-------

bool ParameterCache::isCache(const Util::StringIdentifier& id) {
  return caches.count(id) > 0;
}

//-------

void ParameterCache::bind(const Util::StringIdentifier& id, uint32_t location, uint32_t target) {
  const auto& cache = caches.at(id);
  cache.buffer.bind(target, location);
}

//-------

void ParameterCache::setParameter(const Util::StringIdentifier& id, uint32_t index, const uint8_t* data) {
  auto& cache = caches.at(id);
  if(index >= cache.maxElementCount) {
    WARN("ParameterCache::setParameter: index out of range.");
    return;
  }
  cache.buffer.upload(data, cache.elementSize, index * cache.elementSize);
}

//-------

uint32_t ParameterCache::addParameter(const Util::StringIdentifier& id, const uint8_t* data, bool autoResize) {
  auto& cache = caches.at(id);
  uint32_t index = cache.head++;
  if(index >= cache.maxElementCount) {
    if(autoResize) {
      resizeCache(id, cache.maxElementCount*2);
    } else {
      WARN("ParameterCache::setParameter: cache is full.");
      return INVALID_INDEX;
    }
  }
  cache.buffer.upload(data, cache.elementSize, index * cache.elementSize);
  return index;
}

//-------

} /* Rendering */


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
    CacheEntry entry{id, BufferObject{}, elementSize, maxElementCount};
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

bool ParameterCache::isCache(const Util::StringIdentifier& id) {
  return caches.count(id) > 0;
}

//-------

void ParameterCache::bind(const Util::StringIdentifier& id, uint32_t location, uint32_t target, bool force) {
  auto& cache = caches.at(id);
  if(force || cache.lastBinding != std::make_pair(location, target)) {
    cache.buffer.bind(target, location);
    cache.lastBinding = std::make_pair(location, target);
  }
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

void ParameterCache::wait() {
  lock.waitForLockedRange(0, 1);
}

void ParameterCache::sync() {
  lock.lockRange(0, 1);
}

void ParameterCache::flush() {
  for(auto& e : caches) {
    e.second.buffer.flush();
  }
}

} /* Rendering */


/*
	This file is part of the Rendering library.
	Copyright (C) 2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_PARAMETER_CACHE_H_
#define RENDERING_PARAMETER_CACHE_H_

#include "../Memory/BufferObject.h"
#include "../Memory/BufferLock.h"

#include <Util/StringIdentifier.h>

#include <unordered_map>

namespace Rendering {
/**
 * Manages all parameters used by shaders stored in buffer objects.
 */
class ParameterCache {
private:
  struct CacheEntry {
    CacheEntry(const Util::StringIdentifier& id, uint32_t elementSize, uint32_t maxElementCount) :
      id(id), elementSize(elementSize), maxElementCount(maxElementCount) {}
    Util::StringIdentifier id;
    uint32_t elementSize;
    uint32_t maxElementCount;
    uint32_t head = 0;
    uint32_t multiBufferCount = 1;
    uint32_t multiBufferHead = 0;
    BufferObject buffer;
    BufferLockManager lock;
    std::pair<uint32_t, uint32_t> lastBinding;
  };
  std::unordered_map<Util::StringIdentifier, CacheEntry> caches;
public:
  static const uint32_t INVALID_INDEX;
  
  /**
   * @brief Create a new cache
   * 
   * Allocates a new buffer containing @a maxElementCount elements of size @a elementSize.
   */
  void createCache(const Util::StringIdentifier& id, uint32_t elementSize, uint32_t maxElementCount = 1, uint32_t usageFlags=0, uint32_t multiBufferCount=1);
  
  //! Delete a previously created cache.
  void deleteCache(const Util::StringIdentifier& id);
    
  //! Resizes an existing cache.
  void resizeCache(const Util::StringIdentifier& id, uint32_t elementCount);
  
  //! Returns true if a cache with the given name exists.
  bool isCache(const Util::StringIdentifier& id);
    
  //! Bind a cache to the specified location in a shader.
  void bind(const Util::StringIdentifier& id, uint32_t location, uint32_t target = BufferObject::TARGET_UNIFORM_BUFFER, bool force=false);
  
  //! Sets a parameter in the specified cache
  void setParameter(const Util::StringIdentifier& id, uint32_t index, const uint8_t* data);
  template<typename T>
  void setParameter(const Util::StringIdentifier& id, uint32_t index, const T& parameter) {
    setParameter(id, index, reinterpret_cast<const uint8_t*>(&parameter));
  }
  
  //! Adds a parameter in the specified cache and increases the head counter
  uint32_t addParameter(const Util::StringIdentifier& id, const uint8_t* data);
  template<typename T>
  uint32_t addParameter(const Util::StringIdentifier& id, const T& parameter) {
    return addParameter(id, reinterpret_cast<const uint8_t*>(&parameter));
  }
  
  //! Swaps a multi-buffered cache.
  void swap(const Util::StringIdentifier& id);
};

} /* Rendering */

#endif /* end of include guard: RENDERING_PARAMETER_CACHE_H_ */

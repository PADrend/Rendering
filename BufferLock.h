/*
	This file is part of the Rendering library.
	Copyright (C) 2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_BUFFER_LOCK_H_
#define RENDERING_BUFFER_LOCK_H_

#include <vector>
#include <cstddef>

typedef struct __GLsync *GLsync;

namespace Rendering {
  
struct BufferRange {
  BufferRange(size_t start, size_t length) : start(start), length(length) {}
  size_t start;
  size_t length;
  
  bool overlaps(const BufferRange& other) const {
    return start < (other.start + other.length) && other.start < (start + length);
  }
};
  
struct BufferLock {
  BufferLock(size_t start, size_t length, GLsync sync) : range(start, length), sync(sync) {}
  BufferRange range;
  GLsync sync;
};

class BufferLockManager {
public:
  BufferLockManager() {}
  ~BufferLockManager();
  
  void waitForLockedRange(size_t start, size_t length);
  void lockRange(size_t start, size_t length);
private:
  void wait(GLsync& sync);
  void cleanup(BufferLock& lock);
  std::vector<BufferLock> bufferLocks;
};

} /* Rendering */

#endif /* end of include guard: RENDERING_BUFFER_LOCK_H_ */

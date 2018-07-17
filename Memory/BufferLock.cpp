/*
	This file is part of the Rendering library.
	Copyright (C) 2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "BufferLock.h"
#include "../GLHeader.h"

#include <Util/Macros.h>


namespace Rendering {
  
static const GLuint64 kOneSecondInNanoSeconds = 1000000000;

BufferLockManager::~BufferLockManager() {
  for(auto& lock : bufferLocks)
    cleanup(lock);
}
  
void BufferLockManager::waitForLockedRange(size_t start, size_t length) {
  if(length == 0)
    return;
  BufferRange testRange(start, length);
  std::vector<BufferLock> swapLocks;
  for(auto& lock : bufferLocks) {
    if(testRange.overlaps(lock.range)) {
      wait(lock.sync);
      cleanup(lock);
    } else {
      swapLocks.emplace_back(std::move(lock));
    }
  }
  bufferLocks.swap(swapLocks);
}

void BufferLockManager::lockRange(size_t start, size_t length) {
  if(length > 0) {
    GLsync sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    bufferLocks.emplace_back(start, length, sync);
  }
}

void BufferLockManager::wait(GLsync& sync) {  
  GLbitfield waitFlags = 0;
  GLuint64 waitDuration = 0;
  while(true) {
    GLenum waitRet = glClientWaitSync(sync, waitFlags, waitDuration);
    if (waitRet == GL_ALREADY_SIGNALED || waitRet == GL_CONDITION_SATISFIED) 
      return;

    if (waitRet == GL_WAIT_FAILED) {
      WARN("BufferLockManager: Waiting for buffer lock failed.");
      return;
    }

    // After the first time, need to start flushing, and wait for a looong time.
    waitFlags = GL_SYNC_FLUSH_COMMANDS_BIT;
    waitDuration = kOneSecondInNanoSeconds;
  }
}

void BufferLockManager::cleanup(BufferLock& lock) {
  glDeleteSync(lock.sync);
}
    
} /* Rendering */
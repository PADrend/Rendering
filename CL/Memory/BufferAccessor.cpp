/*
 This file is part of the Rendering library.
 Copyright (C) 2014 Sascha Brandt <myeti@mail.upb.de>

 This library is subject to the terms of the Mozilla Public License, v. 2.0.
 You should have received a copy of the MPL along with this library; see the
 file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef RENDERING_HAS_LIB_OPENCL
#include "BufferAccessor.h"

#include "../CommandQueue.h"

namespace Rendering {
namespace CL {

BufferAccessor::BufferAccessor(Buffer* buffer, CommandQueue* queue) :
		ReferenceCounter_t(), buffer(buffer), queue(queue),
		dataPtr(nullptr), cursor(0), size(buffer->getSize()) {
}

BufferAccessor::BufferAccessor(BufferRef buffer, CommandQueueRef queue) :
		ReferenceCounter_t(), buffer(buffer), queue(queue),
		dataPtr(nullptr), cursor(0), size(buffer->getSize()) {
}

BufferAccessor::~BufferAccessor() {
	if(isValid())
		end();
}

void BufferAccessor::begin(ReadWrite_t readWrite /*= ReadWrite_t::ReadWrite*/) {
	if(isValid()) {
		WARN("Called begin() before end().");
		return;
	}
	cursor = 0;
	dataPtr = static_cast<uint8_t*>(queue->mapBuffer(buffer.get(), true, readWrite, cursor, size));
}

void BufferAccessor::end() {
	if(!isValid()) {
		WARN("Called end() before begin().");
		return;
	}
	queue->unmapMemory(buffer.get(), dataPtr);
	dataPtr = nullptr;
}

} /* namespace CL */
} /* namespace Rendering */
#endif /* RENDERING_HAS_LIB_OPENCL */

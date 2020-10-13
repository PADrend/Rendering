/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef RENDERING_BUFFER_BUFFERALLOCATOR_H_
#define RENDERING_BUFFER_BUFFERALLOCATOR_H_

#include <Util/ReferenceCounter.h>
#include <cstddef>

namespace Rendering {
class BufferObject;
using BufferObjectRef = Util::Reference<BufferObject>;

class BufferAllocator : public Util::ReferenceCounter<BufferAllocator> {
public:
	using Ref = Util::Reference<BufferAllocator>;
	virtual ~BufferAllocator() = default;
	virtual BufferObjectRef allocate(size_t size) = 0;
	virtual void free(BufferObject* buffer) = 0;
};

} /* Rendering */

#endif /* end of include guard: RENDERING_BUFFER_BUFFERALLOCATOR_H_ */
/*
	This file is part of the Rendering library.
	Copyright (C) 2014 Sascha Brandt <myeti@mail.upb.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef RENDERING_HAS_LIB_OPENCL
#ifndef RENDERING_CL_BUFFER_H_
#define RENDERING_CL_BUFFER_H_

#include "Memory.h"

namespace cl {
class Buffer;
}

namespace Rendering {
namespace CL {
class Context;

class Buffer : public Memory {
protected:
	Buffer(cl::Memory* buffer) : Memory(buffer) {}
public:

	Buffer();
	Buffer(Context* context, size_t size, ReadWrite_t readWrite, HostPtr_t hostPtrUsage = None, void* hostPtr = nullptr, ReadWrite_t hostReadWrite = ReadWrite);
//	virtual ~Buffer();
	Buffer(const Buffer& buffer);
//	Buffer(Buffer&& buffer);
//	Buffer& operator=(Buffer&&);

	Buffer* createSubBuffer(ReadWrite_t readWrite, size_t origin, size_t size);
};

} /* namespace CL */
} /* namespace Rendering */

#endif /* RENDERING_CL_BUFFER_H_ */
#endif /* RENDERING_HAS_LIB_OPENCL */

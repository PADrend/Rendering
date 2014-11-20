/*
	This file is part of the Rendering library.
	Copyright (C) 2014 Sascha Brandt <myeti@mail.upb.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef RENDERING_HAS_LIB_OPENCL
#include "Buffer.h"
#include "../Context.h"

#include <CL/cl.hpp>

#include <Util/Macros.h>

namespace Rendering {
namespace CL {

Buffer::Buffer() = default;

Buffer::Buffer(Context* context, size_t size, ReadWrite_t readWrite, HostPtr_t hostPtrUsage /*= None*/, void* hostPtr /*= nullptr*/, ReadWrite_t hostReadWrite /*=ReadWrite*/) {
	cl_mem_flags flags = convertToCLFlags(readWrite, hostPtrUsage, hostReadWrite);
	cl_int err;
	mem.reset(new cl::Buffer(*context->_internal(), flags, size, hostPtr, &err));
	if(err != CL_SUCCESS) {
		WARN("Could not create buffer (" + std::to_string(err) + ")");
		FAIL();
	}
}

Buffer::Buffer(const Buffer& buffer) : Memory(new cl::Buffer(*buffer._internal<cl::Buffer>())) { }

//Buffer::Buffer(Buffer&& buffer) = default;
//
//Buffer& Buffer::operator=(Buffer&&) = default;

//Buffer::~Buffer() = default;

Buffer* Buffer::createSubBuffer(ReadWrite_t readWrite, size_t origin, size_t size) {
	cl_mem_flags flags = 0;
	switch (readWrite) {
		case ReadWrite:
			flags = CL_MEM_READ_WRITE;
			break;
		case ReadOnly:
			flags = CL_MEM_READ_ONLY;
			break;
		case WriteOnly:
			flags = CL_MEM_WRITE_ONLY;
			break;
	}
	cl_int err = 0;
	cl_buffer_region region{origin, size};
	cl::Buffer subbuffer = static_cast<cl::Buffer*>(mem.get())->createSubBuffer(flags, CL_BUFFER_CREATE_TYPE_REGION, &region, &err);
	if(err != CL_SUCCESS) {
		WARN("Could not create subbuffer (" + std::to_string(err) + ")");
		return nullptr;
	}
	Buffer* out = new Buffer();
	out->mem.reset(new cl::Buffer(subbuffer));
	return out;
}

} /* namespace CL */
} /* namespace Rendering */

#endif /* RENDERING_HAS_LIB_OPENCL */

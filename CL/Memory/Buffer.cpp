/*
 * Memory.cpp
 *
 *  Created on: Nov 13, 2014
 *      Author: sascha
 */

#ifdef RENDERING_HAS_LIB_OPENCL
#include "Buffer.h"
#include "../Context.h"

#include <CL/cl.hpp>

#include <Util/Macros.h>

namespace Rendering {
namespace CL {

Buffer::Buffer(Context* context, size_t size, ReadWrite_t readWrite, HostPtr_t hostPtrUsage /*= None*/, void* hostPtr /*= nullptr*/, ReadWrite_t hostReadWrite /*=ReadWrite*/) {
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
	switch (hostPtrUsage) {
		case Use:
			FAIL_IF(hostPtr == nullptr);
			flags |= CL_MEM_USE_HOST_PTR;
			break;
		case Alloc:
			flags |= CL_MEM_ALLOC_HOST_PTR;
			break;
		case Copy:
			FAIL_IF(hostPtr == nullptr);
			flags |= CL_MEM_COPY_HOST_PTR;
			break;
		case AllocAndCopy:
			FAIL_IF(hostPtr == nullptr);
			flags |= CL_MEM_ALLOC_HOST_PTR | CL_MEM_COPY_HOST_PTR;
			break;
		default:
			FAIL_IF(hostPtr != nullptr);
	}
	switch (hostReadWrite) {
		case NoAccess:
			flags |= CL_MEM_HOST_NO_ACCESS;
			break;
		case ReadOnly:
			flags |= CL_MEM_HOST_READ_ONLY;
			break;
		case WriteOnly:
			flags |= CL_MEM_HOST_WRITE_ONLY;
			break;
	}
	cl_int err;
	mem.reset(new cl::Buffer(*context->_internal(), flags, size, hostPtr, &err));
	if(err != CL_SUCCESS) {
		WARN("Could not create buffer (" + std::to_string(err) + ")");
		FAIL();
	}
}

} /* namespace CL */
} /* namespace Rendering */
#endif /* RENDERING_HAS_LIB_OPENCL */

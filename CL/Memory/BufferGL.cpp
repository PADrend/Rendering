/*
 * BufferGL.cpp
 *
 *  Created on: Nov 14, 2014
 *      Author: sascha
 */

#ifdef RENDERING_HAS_LIB_OPENCL
#include "BufferGL.h"
#include "../Context.h"

#include <Util/Macros.h>

#include <CL/cl.hpp>

namespace Rendering {
namespace CL {

BufferGL::BufferGL(Context* context, ReadWrite_t readWrite, uint32_t glHandle) {
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
		cl_int err;
		mem.reset(new cl::BufferGL(*context->_internal(), flags, glHandle, &err));
		if(err != CL_SUCCESS) {
			WARN("Could not create gl buffer (" + std::to_string(err) + ")");
			FAIL();
		}
}

} /* namespace CL */
} /* namespace Rendering */
#endif /* RENDERING_HAS_LIB_OPENCL */

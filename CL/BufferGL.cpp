/*
 * BufferGL.cpp
 *
 *  Created on: Nov 14, 2014
 *      Author: sascha
 */

#include "BufferGL.h"

#include <Util/Macros.h>

namespace Rendering {
namespace CL {

BufferGL::BufferGL(const Context& context, ReadWrite_t readWrite, uint32_t glHandle) {
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
		mem = cl::BufferGL(context.context, flags, glHandle, &err);
		if(err != CL_SUCCESS) {
			WARN("Could not create gl buffer (" + std::to_string(err) + ")");
			FAIL();
		}
}

} /* namespace CL */
} /* namespace Rendering */

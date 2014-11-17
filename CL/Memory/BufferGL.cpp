/*
	This file is part of the Rendering library.
	Copyright (C) 2014 Sascha Brandt <myeti@mail.upb.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
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

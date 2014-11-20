/*
	This file is part of the Rendering library.
	Copyright (C) 2014 Sascha Brandt <myeti@mail.upb.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef RENDERING_HAS_LIB_OPENCL
#ifndef RENDERING_CL_BUFFERGL_H_
#define RENDERING_CL_BUFFERGL_H_

#include "Buffer.h"

#include <Rendering/BufferObject.h>

namespace Rendering {
namespace CL {

class BufferGL: public Buffer {
public:
	BufferGL(Context* context, ReadWrite_t readWrite, uint32_t glHandle);
	BufferGL(Context* context, ReadWrite_t readWrite, const BufferObject& buffer) : BufferGL(context, readWrite, buffer.getGLId()) {};

	BufferGL(const BufferGL& buffer);
};

} /* namespace CL */
} /* namespace Rendering */

#endif /* RENDERING_CL_BUFFERGL_H_ */
#endif /* RENDERING_HAS_LIB_OPENCL */

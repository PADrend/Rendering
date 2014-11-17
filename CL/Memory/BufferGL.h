/*
 * BufferGL.h
 *
 *  Created on: Nov 14, 2014
 *      Author: sascha
 */

#ifdef RENDERING_HAS_LIB_OPENCL
#ifndef BUFFERGL_H_
#define BUFFERGL_H_

#include "Buffer.h"

#include <Rendering/BufferObject.h>

namespace Rendering {
namespace CL {

class BufferGL: public CL::Buffer {
public:
	BufferGL(Context* context, ReadWrite_t readWrite, uint32_t glHandle);
	BufferGL(Context* context, ReadWrite_t readWrite, const BufferObject& buffer) : BufferGL(context, readWrite, buffer.getGLId()) {};
	virtual ~BufferGL() = default;
};

} /* namespace CL */
} /* namespace Rendering */

#endif /* BUFFERGL_H_ */
#endif /* RENDERING_HAS_LIB_OPENCL */

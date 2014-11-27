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

#include <Rendering/BufferObject.h>

namespace cl {
class Buffer;
}

namespace Rendering {
namespace CL {
class Context;

enum class BufferType_t : std::uint8_t { TypeBuffer, TypeBufferGL };

class Buffer : public Memory {
private:
	Buffer(Context* context, cl::Buffer* buffer, BufferType_t type);
public:

	/**
	 * Creates a buffer object.
	 *
	 * @param context A valid OpenCL context used to create the buffer object.
	 * @param size The size in bytes of the buffer memory object to be allocated.
	 * @param readWrite This flag specifies if the memory object will be read or written by a kernel.
	 * @param hostPtrUsage Specifies, how to use the hostPtr (See Memory::HostPtr_t for possible values). This is only valid if hostPtr != nullptr.
	 * @param hostPtr A pointer to the buffer data that may already be allocated by the application. The size of the buffer that host_ptr points to must be greater than or equal to the size bytes.
	 * @param hostReadWrite This flag specifies if the memory object will be read or written by the host.
	 */
	Buffer(Context* context, size_t size, ReadWrite_t readWrite, HostPtr_t hostPtrUsage = HostPtr_t::None, void* hostPtr = nullptr, ReadWrite_t hostReadWrite = ReadWrite_t::ReadWrite);

	/**
	 * Creates an OpenCL buffer object from an OpenGL buffer object.
	 *
	 * @param context A valid OpenCL context created from an OpenGL context.
	 * @param readWrite This flag specifies if the memory object will be read or written by a kernel.
	 * @param glHandle The name of a GL buffer object.
	 */
	Buffer(Context* context, ReadWrite_t readWrite, uint32_t glHandle);

	/**
	 * Creates an OpenCL buffer object from an OpenGL buffer object.
	 *
	 * @param context A valid OpenCL context created from an OpenGL context.
	 * @param readWrite This flag specifies if the memory object will be read or written by a kernel.
	 * @param buffer A GL buffer object.
	 */
	Buffer(Context* context, ReadWrite_t readWrite, const BufferObject& buffer) : Buffer(context, readWrite, buffer.getGLId()) {};

	Buffer(const Buffer& buffer);

	/**
	 *
	 * @param readWrite
	 * @param origin
	 * @param size
	 * @return
	 */
	Buffer* createSubBuffer(ReadWrite_t readWrite, size_t origin, size_t size);

	/**
	 * Returns the type of the buffer (TypeBuffer or TypeBufferGL).
	 * @return The type of the buffer.
	 */
	BufferType_t getType() const { return type; }
private:
	BufferType_t type;
};

} /* namespace CL */
} /* namespace Rendering */

#endif /* RENDERING_CL_BUFFER_H_ */
#endif /* RENDERING_HAS_LIB_OPENCL */

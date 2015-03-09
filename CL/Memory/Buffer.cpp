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
#include "../CLUtils.h"
#include <Util/StringUtils.h>

COMPILER_WARN_PUSH
COMPILER_WARN_OFF(-Wpedantic)
COMPILER_WARN_OFF(-Wold-style-cast)
COMPILER_WARN_OFF(-Wcast-qual)
COMPILER_WARN_OFF(-Wshadow)
COMPILER_WARN_OFF(-Wstack-protector)
#include <CL/cl.hpp>
COMPILER_WARN_POP

#include <Util/Macros.h>

namespace Rendering {
namespace CL {

Buffer::Buffer(Context* _context, cl::Buffer* buffer, BufferType_t _type) : Memory(_context), type(_type) {
	switch(type) {
	case BufferType_t::TypeBuffer:
		mem.reset(new cl::Buffer(*buffer));
		break;
	case BufferType_t::TypeBufferGL:
		mem.reset(new cl::BufferGL(*static_cast<cl::BufferGL*>(buffer)));
		break;
	default:
		mem.reset(new cl::Buffer(*buffer));
		break;
	}
}

Buffer::Buffer(Context* _context, size_t size, ReadWrite_t readWrite, HostPtr_t hostPtrUsage /*= None*/, void* hostPtr /*= nullptr*/, ReadWrite_t hostReadWrite /*=ReadWrite*/) : Memory(_context), type(BufferType_t::TypeBuffer) {
	cl_mem_flags flags = convertToCLFlags(readWrite, hostPtrUsage, hostReadWrite);
	cl_int err;
	mem.reset(new cl::Buffer(*context->_internal(), flags, size, hostPtr, &err));
	THROW_ERROR_IF(err != CL_SUCCESS, "Could not create buffer (" + getErrorString(err) + "[" + Util::StringUtils::toString(err) + "])");
}

Buffer::Buffer(Context* _context, ReadWrite_t readWrite, uint32_t glHandle) : Memory(_context), type(BufferType_t::TypeBufferGL) {
	cl_mem_flags flags = 0;
		switch (readWrite) {
			case ReadWrite_t::ReadWrite:
				flags = CL_MEM_READ_WRITE;
				break;
			case ReadWrite_t::ReadOnly:
				flags = CL_MEM_READ_ONLY;
				break;
			case ReadWrite_t::WriteOnly:
				flags = CL_MEM_WRITE_ONLY;
				break;
			case ReadWrite_t::NoAccess:
			default:
				break;
		}
		cl_int err;
		mem.reset(new cl::BufferGL(*context->_internal(), flags, glHandle, &err));
		THROW_ERROR_IF(err != CL_SUCCESS, "Could not create gl buffer (" + getErrorString(err) + "[" + Util::StringUtils::toString(err) + "])");
}

Buffer::Buffer(const Buffer& buffer) : Memory(buffer.context.get()), type(buffer.type) {
	switch(buffer.type) {
	case BufferType_t::TypeBuffer:
		mem.reset(new cl::Buffer(*buffer._internal<cl::Buffer>()));
		break;
	case BufferType_t::TypeBufferGL:
		mem.reset(new cl::BufferGL(*buffer._internal<cl::BufferGL>()));
		break;
	default:
		mem.reset(new cl::Buffer(*buffer._internal<cl::Buffer>()));
		break;
	}
}

//Buffer::Buffer(Buffer&& buffer) = default;
//
//Buffer& Buffer::operator=(Buffer&&) = default;

//Buffer::~Buffer() = default;

Buffer* Buffer::createSubBuffer(ReadWrite_t readWrite, size_t origin, size_t size) {
	cl_mem_flags flags = 0;
	switch (readWrite) {
		case ReadWrite_t::ReadWrite:
			flags = CL_MEM_READ_WRITE;
			break;
		case ReadWrite_t::ReadOnly:
			flags = CL_MEM_READ_ONLY;
			break;
		case ReadWrite_t::WriteOnly:
			flags = CL_MEM_WRITE_ONLY;
			break;
		case ReadWrite_t::NoAccess:
		default:
			break;
	}
	cl_int err = 0;
	cl_buffer_region region{origin, size};
	cl::Buffer subbuffer = static_cast<cl::Buffer*>(mem.get())->createSubBuffer(flags, CL_BUFFER_CREATE_TYPE_REGION, &region, &err);
	THROW_ERROR_IF(err != CL_SUCCESS, "Could not create subbuffer (" + Util::StringUtils::toString(err) + ")");
	return new Buffer(context.get(), &subbuffer, type);
}

} /* namespace CL */
} /* namespace Rendering */

#endif /* RENDERING_HAS_LIB_OPENCL */

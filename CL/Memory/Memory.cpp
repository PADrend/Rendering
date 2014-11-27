/*
 This file is part of the Rendering library.
 Copyright (C) 2014 Sascha Brandt <myeti@mail.upb.de>

 This library is subject to the terms of the Mozilla Public License, v. 2.0.
 You should have received a copy of the MPL along with this library; see the
 file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef RENDERING_HAS_LIB_OPENCL
#include "Memory.h"

#include "../Context.h"

#include <CL/cl.hpp>

namespace Rendering {
namespace CL {

Memory::Memory(Context* context, cl::Memory* mem) : mem(mem), context(context) {}

Memory::Memory(Context* context) : context(context) {};

Memory::~Memory() = default;

//Memory::Memory(Memory&& buffer) = default;
//
//Memory& Memory::operator=(Memory&&) = default;

Context* Memory::getContext() const {
	return context.get();
}

uint32_t Memory::getFlags() const {
	return mem->getInfo<CL_MEM_FLAGS>();
}

size_t Memory::getSize() const {
	return mem->getInfo<CL_MEM_SIZE>();
}

void* Memory::getHostPtr() const {
	return mem->getInfo<CL_MEM_HOST_PTR>();
}

size_t Memory::getOffset() const {
	return mem->getInfo<CL_MEM_OFFSET>();
}

uint32_t convertToCLFlags(ReadWrite_t readWrite, HostPtr_t hostPtrUsage, ReadWrite_t hostReadWrite) {
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
	}
	switch (hostPtrUsage) {
		case HostPtr_t::Use:
			flags |= CL_MEM_USE_HOST_PTR;
			break;
		case HostPtr_t::Alloc:
			flags |= CL_MEM_ALLOC_HOST_PTR;
			break;
		case HostPtr_t::Copy:
			flags |= CL_MEM_COPY_HOST_PTR;
			break;
		case HostPtr_t::AllocAndCopy:
			flags |= CL_MEM_ALLOC_HOST_PTR | CL_MEM_COPY_HOST_PTR;
			break;
	}
	switch (hostReadWrite) {
		case ReadWrite_t::NoAccess:
			flags |= CL_MEM_HOST_NO_ACCESS;
			break;
		case ReadWrite_t::ReadOnly:
			flags |= CL_MEM_HOST_READ_ONLY;
			break;
		case ReadWrite_t::WriteOnly:
			flags |= CL_MEM_HOST_WRITE_ONLY;
			break;
	}
	return flags;
}


} /* namespace CL */
} /* namespace Rendering */
#endif /* RENDERING_HAS_LIB_OPENCL */

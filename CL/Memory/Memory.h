/*
 This file is part of the Rendering library.
 Copyright (C) 2014 Sascha Brandt <myeti@mail.upb.de>

 This library is subject to the terms of the Mozilla Public License, v. 2.0.
 You should have received a copy of the MPL along with this library; see the
 file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef RENDERING_HAS_LIB_OPENCL
#ifndef RENDERING_CL_MEMORY_H_
#define RENDERING_CL_MEMORY_H_

#include "../CLUtils.h"

#include <Util/ReferenceCounter.h>

#include <memory>

namespace cl {
class Memory;
}

namespace Rendering {
namespace CL {
class Context;

enum class HostPtr_t : std::uint8_t {
	None, //! Ignore the host pointer.
	Use, //! Indicates that the application wants the OpenCL implementation to use memory referenced by the host pointer as the storage bits for the memory object.
	Alloc,
	Copy,
	AllocAndCopy
};

class Memory : public Util::ReferenceCounter<Memory> {
protected:
	Memory(Context* context, cl::Memory* mem);
	Memory(Context* context);
public:
//	Memory(Memory&& buffer);
//	Memory& operator=(Memory&&);
	virtual ~Memory();

	Context* getContext() const;
	uint32_t getFlags() const;
	size_t getSize() const;
	void* getHostPtr() const;
	size_t getOffset() const;

	cl::Memory* _internal() const { return mem.get(); }
	template<class Type>
	Type* _internal() const { return static_cast<Type*>(mem.get()); }
protected:
	std::unique_ptr<cl::Memory> mem;
	ContextRef context;
};

uint32_t convertToCLFlags(ReadWrite_t readWrite, HostPtr_t hostPtrUsage, ReadWrite_t hostReadWrite);

} /* namespace CL */
} /* namespace Rendering */

#endif /* RENDERING_CL_MEMORY_H_ */
#endif /* RENDERING_HAS_LIB_OPENCL */

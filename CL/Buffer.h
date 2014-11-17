/*
 * Memory.h
 *
 *  Created on: Nov 13, 2014
 *      Author: sascha
 */

#ifndef MEMORY_H_
#define MEMORY_H_

#include "Context.h"

#include <CL/cl.hpp>

namespace Rendering {
namespace CL {

class Buffer {
public:
	enum ReadWrite_t { ReadWrite, WriteOnly, ReadOnly, NoAccess };
	enum HostPtr_t { None, Use, Alloc, Copy, AllocAndCopy };

	Buffer() {}
	Buffer(const Context& context, size_t size, ReadWrite_t readWrite, HostPtr_t hostPtrUsage = None, void* hostPtr = nullptr, ReadWrite_t hostReadWrite = ReadWrite);
	virtual ~Buffer() = default;

	cl::Buffer mem;
};

} /* namespace CL */
} /* namespace Rendering */

#endif /* MEMORY_H_ */

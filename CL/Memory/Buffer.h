/*
 * Memory.h
 *
 *  Created on: Nov 13, 2014
 *      Author: sascha
 */
#ifdef RENDERING_HAS_LIB_OPENCL
#ifndef MEMORY_H_
#define MEMORY_H_

#include <memory>

namespace cl {
class Buffer;
}

namespace Rendering {
namespace CL {
class Context;

class Buffer {
public:
	enum ReadWrite_t { ReadWrite, WriteOnly, ReadOnly, NoAccess };
	enum HostPtr_t { None, Use, Alloc, Copy, AllocAndCopy };

	Buffer() {}
	Buffer(Context* context, size_t size, ReadWrite_t readWrite, HostPtr_t hostPtrUsage = None, void* hostPtr = nullptr, ReadWrite_t hostReadWrite = ReadWrite);
	virtual ~Buffer() = default;

	cl::Buffer* _internal() const { return mem.get(); }
protected:
	std::unique_ptr<cl::Buffer> mem;
};

} /* namespace CL */
} /* namespace Rendering */

#endif /* MEMORY_H_ */
#endif /* RENDERING_HAS_LIB_OPENCL */

/*
 * Kernel.h
 *
 *  Created on: Nov 13, 2014
 *      Author: sascha
 */

#ifdef RENDERING_HAS_LIB_OPENCL
#ifndef KERNEL_H_
#define KERNEL_H_

#include <string>
#include <memory>

namespace cl {
class Kernel;
}

namespace Rendering {
namespace CL {
class Program;
class Buffer;

class Kernel {
public:
	Kernel(Program* program, const std::string& name);
	virtual ~Kernel() = default;

	bool setArg(uint32_t index, Buffer* value);
	bool setArg(uint32_t index, float value);

	cl::Kernel* _internal() const { return kernel.get(); }
private:
	std::unique_ptr<cl::Kernel> kernel;
};

} /* namespace CL */
} /* namespace Rendering */

#endif /* KERNEL_H_ */
#endif /* RENDERING_HAS_LIB_OPENCL */

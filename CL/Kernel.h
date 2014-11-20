/*
	This file is part of the Rendering library.
	Copyright (C) 2014 Sascha Brandt <myeti@mail.upb.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef RENDERING_HAS_LIB_OPENCL
#ifndef RENDERING_CL_KERNEL_H_
#define RENDERING_CL_KERNEL_H_

#include <string>
#include <memory>
#include <array>

namespace cl {
class Kernel;
}

namespace Rendering {
namespace CL {
class Program;
class Device;
class Memory;
class Context;

class Kernel {
public:
	Kernel(Program* program, const std::string& name);
	~Kernel();
	Kernel(const Kernel& kernel);
	Kernel(Kernel&& kernel);
	Kernel& operator=(Kernel&&);

	bool setArg(uint32_t index, Memory* value);
	bool setArg(uint32_t index, float value);

	Context* getContext() const;
	Program* getProgram() const;

	std::string getAttributes() const;
	std::string getFunctionName() const;
	uint32_t getNumArgs() const;

	std::string getArgName(uint32_t index) const;
	std::string getArgTypeName(uint32_t index) const;

//	std::array<size_t, 3> getGlobalWorkSize(const Device& device) const;
	size_t getWorkGroupSize(const Device& device) const;
	std::array<size_t, 3> getCompileWorkGroupSize(const Device& device) const;
	uint64_t getLocalMemSize(const Device& device) const;
	size_t getPreferredWorkGroupSizeMultiple(const Device& device) const;
	uint64_t getPrivateMemSize(const Device& device) const;

	cl::Kernel* _internal() const { return kernel.get(); }
private:
	std::unique_ptr<cl::Kernel> kernel;
};

} /* namespace CL */
} /* namespace Rendering */

#endif /* RENDERING_CL_KERNEL_H_ */
#endif /* RENDERING_HAS_LIB_OPENCL */

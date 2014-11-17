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

#endif /* RENDERING_CL_KERNEL_H_ */
#endif /* RENDERING_HAS_LIB_OPENCL */

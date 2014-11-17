/*
 * Kernel.h
 *
 *  Created on: Nov 13, 2014
 *      Author: sascha
 */

#ifndef KERNEL_H_
#define KERNEL_H_

#include "Program.h"
#include "Buffer.h"

#include <CL/cl.hpp>

#include <string>

namespace Rendering {
namespace CL {

class Kernel {
public:
	Kernel(const Program& program, const std::string& name);
	virtual ~Kernel() = default;

	bool setArg(uint32_t index, const Buffer& value);
	bool setArg(uint32_t index, float value);

	cl::Kernel kernel;
};

} /* namespace CL */
} /* namespace Rendering */

#endif /* KERNEL_H_ */

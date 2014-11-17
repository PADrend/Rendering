/*
 * Kernel.cpp
 *
 *  Created on: Nov 13, 2014
 *      Author: sascha
 */

#include "Kernel.h"

#include <Util/Macros.h>

namespace Rendering {
namespace CL {

Kernel::Kernel(const Program& program, const std::string& name) {
	cl_int err;
	kernel = cl::Kernel(program.program, name.c_str(), &err);
	if(err != CL_SUCCESS)
		WARN("Could not create kernel (" + std::to_string(err) + ")");
	FAIL_IF(err != CL_SUCCESS);
}

bool Kernel::setArg(uint32_t index, const Buffer& value) {
	cl_int err = kernel.setArg(static_cast<cl_uint>(index), value.mem);
	if(err != CL_SUCCESS)
		WARN("Could not set kernel argument (" + std::to_string(err) + ")");
	return err == CL_SUCCESS;
}

bool Kernel::setArg(uint32_t index, float value) {
	cl_int err = kernel.setArg(static_cast<cl_uint>(index), value);
	if(err != CL_SUCCESS)
		WARN("Could not set kernel argument (" + std::to_string(err) + ")");
	return err == CL_SUCCESS;
}

} /* namespace CL */
} /* namespace Rendering */

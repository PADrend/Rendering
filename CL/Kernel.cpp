/*
 * Kernel.cpp
 *
 *  Created on: Nov 13, 2014
 *      Author: sascha
 */

#ifdef RENDERING_HAS_LIB_OPENCL
#include "Kernel.h"

#include "Program.h"
#include "Memory/Buffer.h"
#include "CLUtils.h"

#include <CL/cl.hpp>

#include <Util/Macros.h>

namespace Rendering {
namespace CL {

Kernel::Kernel(Program* program, const std::string& name) {
	cl_int err;
	kernel.reset(new cl::Kernel(*program->_internal(), name.c_str(), &err));
	if(err != CL_SUCCESS)
		WARN("Could not create kernel (" + getErrorString(err) + ")");
	FAIL_IF(err != CL_SUCCESS);
}

bool Kernel::setArg(uint32_t index, Buffer* value) {
	cl_int err = kernel->setArg(static_cast<cl_uint>(index), *value->_internal());
	if(err != CL_SUCCESS)
		WARN("Could not set kernel argument (" + getErrorString(err) + ")");
	return err == CL_SUCCESS;
}

bool Kernel::setArg(uint32_t index, float value) {
	cl_int err = kernel->setArg(static_cast<cl_uint>(index), value);
	if(err != CL_SUCCESS)
		WARN("Could not set kernel argument (" + getErrorString(err) + ")");
	return err == CL_SUCCESS;
}

} /* namespace CL */
} /* namespace Rendering */
#endif /* RENDERING_HAS_LIB_OPENCL */

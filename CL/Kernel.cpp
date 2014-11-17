/*
	This file is part of the Rendering library.
	Copyright (C) 2014 Sascha Brandt <myeti@mail.upb.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
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

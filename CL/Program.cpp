/*
	This file is part of the Rendering library.
	Copyright (C) 2014 Sascha Brandt <myeti@mail.upb.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef RENDERING_HAS_LIB_OPENCL
#include "Program.h"

#include "Context.h"
#include "Device.h"
#include "CLUtils.h"

#include <CL/cl.hpp>

#include <Util/Macros.h>

#include <iostream>

namespace Rendering {
namespace CL {

Program::Program(Context* context, const std::string& source) {
	cl_int err;
	program.reset(new cl::Program(*context->_internal(), source, false, &err));
	FAIL_IF(err != CL_SUCCESS);
}

bool Program::build(Device* device, const std::string& parameters /*= ""*/) {
	return build({device}, parameters);
}

bool Program::build(const std::vector<Device*>& devices, const std::string& parameters /*= ""*/) {
	std::vector<cl::Device> cl_devices;
	for(auto device : devices)
		cl_devices.push_back(*device->_internal());
	cl_int err = program->build(cl_devices, parameters.c_str());
	if(err != CL_SUCCESS) {
		WARN("Failed to build program (" + getErrorString(err) + ")");
		for(auto device : devices) {
			std::cerr << "Device: \t" << device->getName() << std::endl;
			std::cerr << "Build Status: " << getBuildStatus(device) << std::endl;
			std::cerr << "Build Options:\t" << getBuildOptions(device) << std::endl;
			std::cerr << "Build Log:\t " << getBuildLog(device) << std::endl;
		}
	}
	return err == CL_SUCCESS;
}

Program::BuildStatus_t Program::getBuildStatus(Device* device) const {
	cl_int status = program->getBuildInfo<CL_PROGRAM_BUILD_STATUS>(*device->_internal());
	switch (status) {
		case CL_BUILD_SUCCESS:
			return Success;
		case CL_BUILD_IN_PROGRESS:
			return InProgress;
		case CL_BUILD_ERROR:
			return Error;
		default:
			return None;
	}
}

std::string Program::getBuildOptions(Device* device) const {
	return program->getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>(*device->_internal());
}

std::string Program::getBuildLog(Device* device) const {
	return program->getBuildInfo<CL_PROGRAM_BUILD_LOG>(*device->_internal());
}

} /* namespace CL */
} /* namespace Rendering */
#endif /* RENDERING_HAS_LIB_OPENCL */

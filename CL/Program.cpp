/*
 * Program.cpp
 *
 *  Created on: Nov 13, 2014
 *      Author: sascha
 */

#include "Program.h"

#include <Util/Macros.h>

namespace Rendering {
namespace CL {

Program::Program(const Context& context, const std::string& source) {
	cl_int err;
	program = cl::Program(context.context, source, false, &err);
	FAIL_IF(err != CL_SUCCESS);
}

bool Program::build(const std::vector<Device>& devices, const std::string& parameters /*= ""*/) {
	std::vector<cl::Device> cl_devices;
	for(auto device : devices)
		cl_devices.push_back(device.device);
	cl_int err = program.build(cl_devices, parameters.c_str());
	if(err != CL_SUCCESS)
		WARN("Failed to build program (" + std::to_string(err) + ")");
	return err == CL_SUCCESS;
}

Program::BuildStatus_t Program::getBuildStatus(const Device& device) {
	cl_int status = program.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(device.device);
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

std::string Program::getBuildOptions(const Device& device) {
	return program.getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>(device.device);
}

std::string Program::getBuildLog(const Device& device) {
	return program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device.device);
}

} /* namespace CL */
} /* namespace Rendering */

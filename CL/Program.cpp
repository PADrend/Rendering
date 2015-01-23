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
#include <Util/IO/FileUtils.h>

#include <iostream>

namespace Rendering {
namespace CL {

Program::Program(Context* context) : context(context) {
	cl_int err;
	program.reset(new cl::Program(*context->_internal(), "", &err));
	FAIL_IF(err != CL_SUCCESS);
}

Program::Program(Context* context, const std::vector<std::string>& sources) : context(context) {
	cl_int err;
	cl::Program::Sources cl_sources;
	for(auto src : sources)
		cl_sources.push_back(std::make_pair(src.c_str(),src.size()));
	program.reset(new cl::Program(*context->_internal(), cl_sources, &err));
	FAIL_IF(err != CL_SUCCESS);
}

Program::~Program() = default;

Program::Program(const Program& program) : program(new cl::Program(*program.program.get())), context(program.context) { }

//Program::Program(Program&& program) = default;

//Program& Program::operator=(Program&&) = default;

bool Program::build(const std::vector<DeviceRef>& devices, const std::string& options /*= ""*/) {
	std::vector<cl::Device> cl_devices;
	for(auto device : devices)
		cl_devices.push_back(*device->_internal());
	cl_int err = program->build(cl_devices, options.c_str());
	if(err != CL_SUCCESS) {
		WARN("Failed to build program (" + getErrorString(err) + ")");
		for(auto device : devices) {
			std::cerr << "Device: \t" << device->getName() << std::endl;
			std::cerr << "Build Status: " << static_cast<uint32_t>(getBuildStatus(device.get())) << std::endl;
			std::cerr << "Build Options:\t" << getBuildOptions(device.get()) << std::endl;
			std::cerr << "Build Log:\t " << getBuildLog(device.get()) << std::endl;
		}
	}
	return err == CL_SUCCESS;
}

BuildStatus_t Program::getBuildStatus(Device* device) const {
	cl_int status = program->getBuildInfo<CL_PROGRAM_BUILD_STATUS>(*device->_internal());
	switch (status) {
		case CL_BUILD_SUCCESS:
			return BuildStatus_t::Success;
		case CL_BUILD_IN_PROGRESS:
			return BuildStatus_t::InProgress;
		case CL_BUILD_ERROR:
			return BuildStatus_t::Error;
		default:
			return BuildStatus_t::None;
	}
}

std::string Program::getBuildOptions(Device* device) const {
	return program->getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>(*device->_internal());
}

std::string Program::getBuildLog(Device* device) const {
	return program->getBuildInfo<CL_PROGRAM_BUILD_LOG>(*device->_internal());
}

std::vector<char*> Program::getBinaries() const {
	return program->getInfo<CL_PROGRAM_BINARIES>();
}

std::vector<size_t> Program::getBinarySizes() const {
	return program->getInfo<CL_PROGRAM_BINARY_SIZES>();
}

std::vector<DeviceRef> Program::getDevices() const {
	std::vector<DeviceRef> out;
	std::vector<cl::Device> cl_devices = program->getInfo<CL_PROGRAM_DEVICES>();
	for(auto dev : cl_devices)
		out.push_back(new Device(context->getPlatform(), &dev));
	return out;
}

uint32_t Program::getNumDevices() const {
	return program->getInfo<CL_PROGRAM_NUM_DEVICES>();
}

std::string Program::getKernelNames() const {
	return program->getInfo<CL_PROGRAM_KERNEL_NAMES>();
}

uint32_t Program::getNumKernels() const {
	return program->getInfo<CL_PROGRAM_NUM_KERNELS>();
}

std::string Program::getSource() const {
	return program->getInfo<CL_PROGRAM_SOURCE>();
}


void Program::attachSource(const std::string& source) {
	sources.push_back(source);

	cl_int err;
	cl::Program::Sources cl_sources;
	for(auto src : sources)
		cl_sources.push_back(std::make_pair(src.c_str(),src.size()));
	program.reset(new cl::Program(*context->_internal(), cl_sources, &err));
	FAIL_IF(err != CL_SUCCESS);
}

void Program::attachSource(const Util::FileName& file) {
	attachSource(Util::FileUtils::getFileContents(file));
}

} /* namespace CL */
} /* namespace Rendering */


#endif /* RENDERING_HAS_LIB_OPENCL */

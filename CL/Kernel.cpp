/*
	This file is part of the Rendering library.
	Copyright (C) 2014 Sascha Brandt <myeti@mail.upb.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef RENDERING_HAS_LIB_OPENCL
#include "Kernel.h"

#include "Context.h"
#include "Program.h"
#include "Device.h"
#include "Memory/Memory.h"
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

Kernel::Kernel(const Kernel& kernel) : kernel(new cl::Kernel(*kernel.kernel.get())) { }

Kernel::Kernel(Kernel&& kernel) = default;

Kernel::~Kernel() = default;

Kernel& Kernel::operator=(Kernel&&) = default;

bool Kernel::setArg(uint32_t index, Memory* value) {
	cl_int err = kernel->setArg(index, *value->_internal());
	if(err != CL_SUCCESS)
		WARN("Could not set kernel argument (" + getErrorString(err) + ")");
	return err == CL_SUCCESS;
}

bool Kernel::setArg(uint32_t index, float value) {
	cl_int err = kernel->setArg(index, value);
	if(err != CL_SUCCESS)
		WARN("Could not set kernel argument (" + getErrorString(err) + ")");
	return err == CL_SUCCESS;
}

Context* Kernel::getContext() const {
	return nullptr;
}

Program* Kernel::getProgram() const {
	return nullptr;
}

std::string Kernel::getAttributes() const {
	return kernel->getInfo<CL_KERNEL_ATTRIBUTES>();
}

std::string Kernel::getFunctionName() const {
	return kernel->getInfo<CL_KERNEL_FUNCTION_NAME>();
}

uint32_t Kernel::getNumArgs() const {
	return kernel->getInfo<CL_KERNEL_NUM_ARGS>();
}

std::string Kernel::getArgName(uint32_t index) const {
	return kernel->getArgInfo<CL_KERNEL_ARG_NAME>(index);
}

std::string Kernel::getArgTypeName(uint32_t index) const {
	return kernel->getArgInfo<CL_KERNEL_ARG_TYPE_NAME>(index);
}

//std::array<size_t, 3> Kernel::getGlobalWorkSize(const Device& device) const {
//	return kernel->getWorkGroupInfo<CL_KERNEL_GLOBAL_WORK_SIZE>(*device._internal());
//}

size_t Kernel::getWorkGroupSize(const Device& device) const {
	return kernel->getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>(*device._internal());
}

std::array<size_t, 3> Kernel::getCompileWorkGroupSize(const Device& device) const {
	auto sizes = kernel->getWorkGroupInfo<CL_KERNEL_COMPILE_WORK_GROUP_SIZE>(*device._internal());
	std::array<size_t, 3> out;
	out[0] = sizes[0];
	out[1] = sizes[1];
	out[2] = sizes[2];
	return out;
}

uint64_t Kernel::getLocalMemSize(const Device& device) const {
	return kernel->getWorkGroupInfo<CL_KERNEL_LOCAL_MEM_SIZE>(*device._internal());
}

size_t Kernel::getPreferredWorkGroupSizeMultiple(const Device& device) const {
	return kernel->getWorkGroupInfo<CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE>(*device._internal());
}

uint64_t Kernel::getPrivateMemSize(const Device& device) const {
	return kernel->getWorkGroupInfo<CL_KERNEL_PRIVATE_MEM_SIZE>(*device._internal());
}

} /* namespace CL */
} /* namespace Rendering */

#endif /* RENDERING_HAS_LIB_OPENCL */

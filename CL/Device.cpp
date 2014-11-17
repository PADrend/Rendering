/*
 * Device.cpp
 *
 *  Created on: Nov 12, 2014
 *      Author: sascha
 */

#include "Device.h"

namespace Rendering {
namespace CL {

const uint32_t Device::TYPE_DEFAULT = CL_DEVICE_TYPE_DEFAULT;
const uint32_t Device::TYPE_CPU = CL_DEVICE_TYPE_CPU;
const uint32_t Device::TYPE_GPU = CL_DEVICE_TYPE_GPU;
const uint32_t Device::TYPE_ACCELERATOR = CL_DEVICE_TYPE_ACCELERATOR;
const uint32_t Device::TYPE_CUSTOM = CL_DEVICE_TYPE_CUSTOM;
const uint32_t Device::TYPE_ALL = CL_DEVICE_TYPE_ALL;

Device::Device(const cl::Device& device) : device(device) {
}

std::string Device::getBuiltInKernels() const {
	return device.getInfo<CL_DEVICE_BUILT_IN_KERNELS>();
}

std::string Device::getExtensions() const {
	return device.getInfo<CL_DEVICE_EXTENSIONS>();
}

std::string Device::getName() const {
	return device.getInfo<CL_DEVICE_NAME>();
}

std::string Device::getOpenCL_CVersion() const {
	return device.getInfo<CL_DEVICE_OPENCL_C_VERSION>();
}

std::string Device::getProfile() const {
	return device.getInfo<CL_DEVICE_PROFILE>();
}

std::string Device::getVendor() const {
	return device.getInfo<CL_DEVICE_VENDOR>();
}

std::string Device::getVersion() const {
	return device.getInfo<CL_DEVICE_VERSION>();
}

std::string Device::getDriverVersion() const {
	return device.getInfo<CL_DRIVER_VERSION>();
}

std::vector<size_t> Device::getMaxWorkItemSizes() const {
	return device.getInfo<CL_DEVICE_MAX_WORK_ITEM_SIZES>();
}

uint32_t Device::getType() const {
	return static_cast<uint32_t>(device.getInfo<CL_DEVICE_TYPE>());
}

} /* namespace CL */
} /* namespace Rendering */

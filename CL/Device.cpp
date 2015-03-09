/*
	This file is part of the Rendering library.
	Copyright (C) 2014 Sascha Brandt <myeti@mail.upb.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef RENDERING_HAS_LIB_OPENCL
#include "Device.h"

#include "CLUtils.h"

#include <Util/Macros.h>

COMPILER_WARN_PUSH
COMPILER_WARN_OFF(-Wpedantic)
COMPILER_WARN_OFF(-Wold-style-cast)
COMPILER_WARN_OFF(-Wcast-qual)
COMPILER_WARN_OFF(-Wshadow)
COMPILER_WARN_OFF(-Wstack-protector)
#include <CL/cl.hpp>
COMPILER_WARN_POP

namespace Rendering {
namespace CL {

const uint32_t Device::TYPE_DEFAULT = CL_DEVICE_TYPE_DEFAULT;
const uint32_t Device::TYPE_CPU = CL_DEVICE_TYPE_CPU;
const uint32_t Device::TYPE_GPU = CL_DEVICE_TYPE_GPU;
const uint32_t Device::TYPE_ACCELERATOR = CL_DEVICE_TYPE_ACCELERATOR;
const uint32_t Device::TYPE_CUSTOM = CL_DEVICE_TYPE_CUSTOM;
const uint32_t Device::TYPE_ALL = CL_DEVICE_TYPE_ALL;

//Device::Device() = default;

Device::Device(Platform* _platform, cl::Device* _device) : Util::ReferenceCounter<Device>(), device(new cl::Device(*_device)), platform(_platform) { }

Device::~Device() = default;

Device::Device(const Device& _device) : Util::ReferenceCounter<Device>(), device(new cl::Device(*_device.device.get())) { }

//Device::Device(Device&& device) = default;
//
//Device& Device::operator=(Device&&) = default;

std::string Device::getBuiltInKernels() const {
	return device->getInfo<CL_DEVICE_BUILT_IN_KERNELS>();
}

std::string Device::getExtensions() const {
	return device->getInfo<CL_DEVICE_EXTENSIONS>();
}

std::string Device::getName() const {
	return device->getInfo<CL_DEVICE_NAME>();
}

std::string Device::getOpenCL_CVersion() const {
	return device->getInfo<CL_DEVICE_OPENCL_C_VERSION>();
}

std::string Device::getProfile() const {
	return device->getInfo<CL_DEVICE_PROFILE>();
}

std::string Device::getVendor() const {
	return device->getInfo<CL_DEVICE_VENDOR>();
}

std::string Device::getVersion() const {
	return device->getInfo<CL_DEVICE_VERSION>();
}

std::string Device::getDriverVersion() const {
	return device->getInfo<CL_DRIVER_VERSION>();
}

std::vector<size_t> Device::getMaxWorkItemSizes() const {
	return device->getInfo<CL_DEVICE_MAX_WORK_ITEM_SIZES>();
}

uint32_t Device::getType() const {
	return static_cast<uint32_t>(device->getInfo<CL_DEVICE_TYPE>());
}

uint32_t Device::getAddressBits() const {
	return device->getInfo<CL_DEVICE_ADDRESS_BITS>();
}

bool Device::isAvailable() const {
	return device->getInfo<CL_DEVICE_AVAILABLE>();
}

bool Device::isCompilerAvailable() const {
	return device->getInfo<CL_DEVICE_COMPILER_AVAILABLE>();
}

uint32_t Device::getDoubleFPConfig() const {
	return device->getInfo<CL_DEVICE_DOUBLE_FP_CONFIG>();
}

bool Device::isEndianLittle() const {
	return device->getInfo<CL_DEVICE_ENDIAN_LITTLE>();
}

bool Device::isErrorCorrectionSupported() const {
	return device->getInfo<CL_DEVICE_ERROR_CORRECTION_SUPPORT>();
}

uint32_t Device::getExecutionCapabilities() const {
	return device->getInfo<CL_DEVICE_EXECUTION_CAPABILITIES>();
}

size_t Device::getGlobalMemCacheSize() const {
	return device->getInfo<CL_DEVICE_GLOBAL_MEM_CACHE_SIZE>();
}

Device::CacheType_t Device::getGlobalMemCacheType() const {
	cl_device_mem_cache_type type = device->getInfo<CL_DEVICE_GLOBAL_MEM_CACHE_TYPE>();
	switch (type) {
	case CL_READ_ONLY_CACHE:
		return ReadOnly;
	case CL_READ_WRITE_CACHE:
		return ReadWrite;
	default:
		return NoCache;
	}
}

uint32_t Device::getGlobalMemCachelineSize() const {
	return device->getInfo<CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE>();
}

size_t Device::getGlobalMemSize() const {
	return device->getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>();
}

uint32_t Device::getHalfFPConfig() const {
	return device->getInfo<CL_DEVICE_HALF_FP_CONFIG>();
}

bool Device::hasHostUnifiedMemory() const {
	return device->getInfo<CL_DEVICE_HOST_UNIFIED_MEMORY>();
}

bool Device::isImageSupported() const {
	return device->getInfo<CL_DEVICE_IMAGE_SUPPORT>();
}

size_t Device::getImage2DMaxHeight() const {
	return device->getInfo<CL_DEVICE_IMAGE2D_MAX_HEIGHT>();
}

size_t Device::getImage2DMaxWidth() const {
	return device->getInfo<CL_DEVICE_IMAGE2D_MAX_WIDTH>();
}

size_t Device::getImage3DMaxDepth() const {
	return device->getInfo<CL_DEVICE_IMAGE3D_MAX_DEPTH>();
}

size_t Device::getImage3DMaxHeight() const {
	return device->getInfo<CL_DEVICE_IMAGE3D_MAX_HEIGHT>();
}

size_t Device::getImage3DMaxWidth() const {
	return device->getInfo<CL_DEVICE_IMAGE3D_MAX_WIDTH>();
}

//size_t Device::getImageMaxBufferSize() const {
//	return device->getInfo<CL_DEVICE_IMAGE_MAX_BUFFER_SIZE>();
//}
//
//size_t Device::getImageMaxArraySize() const {
//	return device->getInfo<CL_DEVICE_IMAGE_MAX_ARRAY_SIZE>();
//}
//
//bool Device::isLinkerAvailable() const {
//	return device->getInfo<CL_DEVICE_LINKER_AVAILABLE>();
//}

size_t Device::getLocalMemSize() const {
	return device->getInfo<CL_DEVICE_LOCAL_MEM_SIZE>();
}

Device::MemType_t Device::getLocalMemType() const {
	cl_device_local_mem_type type = device->getInfo<CL_DEVICE_LOCAL_MEM_TYPE>();
	switch (type) {
	case CL_LOCAL:
		return Local;
	case CL_GLOBAL:
		return Global;
	default:
		return NoMem;
	}
}

uint32_t Device::getMaxClockFrequency() const {
	return device->getInfo<CL_DEVICE_MAX_CLOCK_FREQUENCY>();
}

uint32_t Device::getMaxComputeUnits() const {
	return device->getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
}

uint32_t Device::getMaxConstantArgs() const {
	return device->getInfo<CL_DEVICE_MAX_CONSTANT_ARGS>();
}

size_t Device::getMaxConstantBufferSize() const {
	return device->getInfo<CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE>();
}

size_t Device::getMaxMemAllocSize() const {
	return device->getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>();
}

size_t Device::getMaxParameterSize() const {
	return device->getInfo<CL_DEVICE_MAX_PARAMETER_SIZE>();
}

uint32_t Device::getMaxReadImageArgs() const {
	return device->getInfo<CL_DEVICE_MAX_READ_IMAGE_ARGS>();
}

uint32_t Device::getMaxSamplers() const {
	return device->getInfo<CL_DEVICE_MAX_SAMPLERS>();
}

size_t Device::getMaxWorkGroupSize() const {
	return device->getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>();
}

uint32_t Device::getMaxWorkItemDimensions() const {
	return device->getInfo<CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS>();
}

uint32_t Device::getMaxWriteImageArgs() const {
	return device->getInfo<CL_DEVICE_MAX_WRITE_IMAGE_ARGS>();
}

uint32_t Device::getMemBaseAddrAlign() const {
	return device->getInfo<CL_DEVICE_MEM_BASE_ADDR_ALIGN>();
}

uint32_t Device::getMinDataTypeAlignSize() const {
	return device->getInfo<CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE>();
}

uint32_t Device::getNativeVectorWidthChar() const {
	return device->getInfo<CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR>();
}

uint32_t Device::getNativeVectorWidthShort() const {
	return device->getInfo<CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT>();
}

uint32_t Device::getNativeVectorWidthInt() const {
	return device->getInfo<CL_DEVICE_NATIVE_VECTOR_WIDTH_INT>();
}

uint32_t Device::getNativeVectorWidthLong() const {
	return device->getInfo<CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG>();
}

uint32_t Device::getNativeVectorWidthFloat() const {
	return device->getInfo<CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT>();
}

uint32_t Device::getNativeVectorWidthDouble() const {
	return device->getInfo<CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE>();
}

uint32_t Device::getNativeVectorWidthHalf() const {
	return device->getInfo<CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF>();
}

Device* Device::getParentDevice() const {
	return new Device(platform.get(), new cl::Device(device->getInfo<CL_DEVICE_PARENT_DEVICE>()));
}

//uint32_t Device::getPartitionMaxSubDevices() const {
//	return device->getInfo<CL_DEVICE_PARTITION_MAX_SUB_DEVICES>();
//}

std::vector<intptr_t> Device::getPartitionProperties() const {
	return device->getInfo<CL_DEVICE_PARTITION_PROPERTIES>();
}

uint32_t Device::getPartitionAffinityDomain() const {
	return device->getInfo<CL_DEVICE_PARTITION_AFFINITY_DOMAIN>();
}

std::vector<intptr_t> Device::getPartitionType() const {
	return device->getInfo<CL_DEVICE_PARTITION_TYPE>();
}

Platform* Device::getPlatform() const {
	return platform.get();
}

uint32_t Device::getPreferredVectorWidthChar() const {
	return device->getInfo<CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR>();
}

uint32_t Device::getPreferredVectorWidthShort() const {
	return device->getInfo<CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT>();
}

uint32_t Device::getPreferredVectorWidthInt() const {
	return device->getInfo<CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT>();
}

uint32_t Device::getPreferredVectorWidthLong() const {
	return device->getInfo<CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG>();
}

uint32_t Device::getPreferredVectorWidthFloat() const {
	return device->getInfo<CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT>();
}

uint32_t Device::getPreferredVectorWidthDouble() const {
	return device->getInfo<CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE>();
}

uint32_t Device::getPreferredVectorWidthHalf() const {
	return device->getInfo<CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF>();
}

//size_t Device::getPrintfBufferSize() const {
//	return device->getInfo<CL_DEVICE_PRINTF_BUFFER_SIZE>();
//}

bool Device::isInteropUserSyncPreferred() const {
	return device->getInfo<CL_DEVICE_PREFERRED_INTEROP_USER_SYNC>();
}

size_t Device::getProfilingTimerResolution() const {
	return device->getInfo<CL_DEVICE_PROFILING_TIMER_RESOLUTION>();
}

uint32_t Device::getQueueProperties() const {
	return device->getInfo<CL_DEVICE_QUEUE_PROPERTIES>();
}

//uint32_t Device::getReferenceCount() const {
//	return device->getInfo<CL_DEVICE_REFERENCE_COUNT>();
//}

uint32_t Device::getSingleFPConfig() const {
	return device->getInfo<CL_DEVICE_SINGLE_FP_CONFIG>();
}

uint32_t Device::getVendorId() const {
	return device->getInfo<CL_DEVICE_VENDOR_ID>();
}


std::vector<DeviceRef> Device::createSubDevices(const std::vector<intptr_t>& properties) {
	std::vector<DeviceRef> out;
	std::vector<cl::Device> cl_devices;
	cl_int err = device->createSubDevices(properties.data(), &cl_devices);
	if(err != CL_SUCCESS) {
		WARN("Could not create subdivices (" + getErrorString(err) + ")");
	} else {
		for(auto dev : cl_devices) {
			out.push_back(new Device(platform.get(), &dev));
		}
	}
	return out;
}

} /* namespace CL */
} /* namespace Rendering */


#endif /* RENDERING_HAS_LIB_OPENCL */

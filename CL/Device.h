/*
	This file is part of the Rendering library.
	Copyright (C) 2014 Sascha Brandt <myeti@mail.upb.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef RENDERING_HAS_LIB_OPENCL
#ifndef RENDERING_CL_DEVICE_H_
#define RENDERING_CL_DEVICE_H_

#include "CLUtils.h"

#include <Util/ReferenceCounter.h>

#include "Platform.h"

#include <vector>
#include <string>
#include <memory>

namespace cl {
class Device;
}

namespace Rendering {
namespace CL {

class Device : public Util::ReferenceCounter<Device> {
public:
	enum CacheType_t {NoCache, ReadOnly, ReadWrite};
	enum MemType_t {NoMem, Local, Global};

	Device();
	Device(cl::Device* device);
	~Device();
	Device(const Device& device);
//	Device(Device&& device);
//	Device& operator=(Device&&);

    static const uint32_t TYPE_DEFAULT;
    static const uint32_t TYPE_CPU;
    static const uint32_t TYPE_GPU;
    static const uint32_t TYPE_ACCELERATOR;
    static const uint32_t TYPE_CUSTOM;
    static const uint32_t TYPE_ALL;

    uint32_t getAddressBits() const;
    bool isAvailable() const;
	std::string getBuiltInKernels() const;
	bool isCompilerAvailable() const;
	uint32_t getDoubleFPConfig() const; //TODO: use string or bitfield
	bool isEndianLittle() const;
	bool isErrorCorrectionSupported() const;
	uint32_t getExecutionCapabilities() const; //TODO: use string or bitfield
	std::string getExtensions() const;
	size_t getGlobalMemCacheSize() const;
	CacheType_t getGlobalMemCacheType() const;
	uint32_t getGlobalMemCachelineSize() const;
	size_t getGlobalMemSize() const;
	uint32_t getHalfFPConfig() const; //TODO: use string or bitfield
	bool hasHostUnifiedMemory() const;
	bool isImageSupported() const;
	size_t getImage2DMaxHeight() const;
	size_t getImage2DMaxWidth() const;
	size_t getImage3DMaxDepth() const;
	size_t getImage3DMaxHeight() const;
	size_t getImage3DMaxWidth() const;
//	size_t getImageMaxBufferSize() const;
//	size_t getImageMaxArraySize() const;
//	bool isLinkerAvailable() const;
	size_t getLocalMemSize() const;
	MemType_t getLocalMemType() const;
	uint32_t getMaxClockFrequency() const;
	uint32_t getMaxComputeUnits() const;
	uint32_t getMaxConstantArgs() const;
	size_t getMaxConstantBufferSize() const;
	size_t getMaxMemAllocSize() const;
	size_t getMaxParameterSize() const;
	uint32_t getMaxReadImageArgs() const;
	uint32_t getMaxSamplers() const;
	size_t getMaxWorkGroupSize() const;
	uint32_t getMaxWorkItemDimensions() const;
	std::vector<size_t> getMaxWorkItemSizes() const;
	uint32_t getMaxWriteImageArgs() const;
	uint32_t getMemBaseAddrAlign() const;
	uint32_t getMinDataTypeAlignSize() const;
	std::string getName() const;
	uint32_t getNativeVectorWidthChar() const;
	uint32_t getNativeVectorWidthShort() const;
	uint32_t getNativeVectorWidthInt() const;
	uint32_t getNativeVectorWidthLong() const;
	uint32_t getNativeVectorWidthFloat() const;
	uint32_t getNativeVectorWidthDouble() const;
	uint32_t getNativeVectorWidthHalf() const;
	std::string getOpenCL_CVersion() const;
	Device getParentDevice() const;
//	uint32_t getPartitionMaxSubDevices() const;
	std::vector<intptr_t> getPartitionProperties() const;  //TODO: use enums
	uint32_t getPartitionAffinityDomain() const;  //TODO: use string or bitfield
	std::vector<intptr_t> getPartitionType() const;  //TODO: use enums
	Platform getPlatform() const;
	uint32_t getPreferredVectorWidthChar() const;
	uint32_t getPreferredVectorWidthShort() const;
	uint32_t getPreferredVectorWidthInt() const;
	uint32_t getPreferredVectorWidthLong() const;
	uint32_t getPreferredVectorWidthFloat() const;
	uint32_t getPreferredVectorWidthDouble() const;
	uint32_t getPreferredVectorWidthHalf() const;
//	size_t getPrintfBufferSize() const;
	bool isInteropUserSyncPreferred() const;
	std::string getProfile() const;
	size_t getProfilingTimerResolution() const;
	uint32_t getQueueProperties() const; //TODO: use string or bitfield
//	uint32_t getReferenceCount() const;
	uint32_t getSingleFPConfig() const; //TODO: use string or bitfield
	uint32_t getType() const;
	std::string getVendor() const;
	uint32_t getVendorId() const;
	std::string getVersion() const;
	std::string getDriverVersion() const;

	std::vector<DeviceRef> createSubDevices(const std::vector<intptr_t>& properties);

	cl::Device* _internal() const { return device.get(); }
private:
	std::unique_ptr<cl::Device> device;
};

} /* namespace CL */
} /* namespace Rendering */

#endif /* RENDERING_CL_DEVICE_H_ */
#endif /* RENDERING_HAS_LIB_OPENCL */

/*
 * Device.h
 *
 *  Created on: Nov 12, 2014
 *      Author: sascha
 */

#ifndef DEVICE_H_
#define DEVICE_H_

#include <vector>
#include <string>

#include <CL/cl.hpp>

namespace Rendering {
namespace CL {

class Device {
public:
	Device(const cl::Device& device);
	virtual ~Device() = default;

    static const uint32_t TYPE_DEFAULT;
    static const uint32_t TYPE_CPU;
    static const uint32_t TYPE_GPU;
    static const uint32_t TYPE_ACCELERATOR;
    static const uint32_t TYPE_CUSTOM;
    static const uint32_t TYPE_ALL;

	std::string getBuiltInKernels() const;
	std::string getExtensions() const;
	std::string getName() const;
	std::string getOpenCL_CVersion() const;
	std::string getProfile() const;
	std::string getVendor() const;
	std::string getVersion() const;
	std::string getDriverVersion() const;
	std::vector<size_t> getMaxWorkItemSizes() const;
	uint32_t getType() const;

	cl::Device device;
};

} /* namespace CL */
} /* namespace Rendering */

#endif /* DEVICE_H_ */

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

#include <vector>
#include <string>
#include <memory>

namespace cl {
class Device;
}

namespace Rendering {
namespace CL {

class Device {
public:
	Device(cl::Device* device);
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

	cl::Device* _internal() const { return device.get(); }
private:
	std::unique_ptr<cl::Device> device;
};

} /* namespace CL */
} /* namespace Rendering */

#endif /* RENDERING_CL_DEVICE_H_ */
#endif /* RENDERING_HAS_LIB_OPENCL */

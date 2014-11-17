/*
 * Platform.cpp
 *
 *  Created on: Nov 12, 2014
 *      Author: sascha
 */

#include "Platform.h"

#include <Util/Macros.h>

#include <CL/cl.hpp>

namespace Rendering {
namespace CL {

Platform::Platform(const cl::Platform& platform) : platform(platform) {
}

std::string Platform::getExtensions() const {
	return platform.getInfo<CL_PLATFORM_EXTENSIONS>();
}

std::string Platform::getName() const {
	return platform.getInfo<CL_PLATFORM_NAME>();
}

std::string Platform::getProfile() const {
	return platform.getInfo<CL_PLATFORM_PROFILE>();
}

std::string Platform::getVendor() const {
	return platform.getInfo<CL_PLATFORM_VENDOR>();
}

std::string Platform::getVersion() const {
	return platform.getInfo<CL_PLATFORM_VERSION>();
}

std::vector<Device> Platform::getDevices() const {
	std::vector<Device> out;
	std::vector<cl::Device> devices;
	platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);
	for(auto device : devices)
		out.push_back(device);
	return out;
}

void Platform::get(std::vector<Platform>* platforms) {
	FAIL_IF(platforms == nullptr);

	std::vector<cl::Platform> out;
	cl::Platform::get(&out);
	for(auto pf : out)
		platforms->push_back(pf);
}

} /* namespace CL */
} /* namespace Rendering */

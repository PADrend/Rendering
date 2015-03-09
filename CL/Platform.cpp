/*
	This file is part of the Rendering library.
	Copyright (C) 2014 Sascha Brandt <myeti@mail.upb.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef RENDERING_HAS_LIB_OPENCL
#include "Platform.h"

#include "Device.h"


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

//Platform::Platform() = default;

Platform::Platform(cl::Platform* _platform) : Util::ReferenceCounter<Platform>(), platform(new cl::Platform(*_platform)) { }

Platform::~Platform() = default;

Platform::Platform(const Platform& _platform) : Util::ReferenceCounter<Platform>(), platform(new cl::Platform(*_platform.platform.get())) { }

//Platform::Platform(Platform&& platform) = default;

//Platform& Platform::operator=(Platform&&) = default;

std::string Platform::getExtensions() const {
	return platform->getInfo<CL_PLATFORM_EXTENSIONS>();
}

std::string Platform::getName() const {
	return platform->getInfo<CL_PLATFORM_NAME>();
}

std::string Platform::getProfile() const {
	return platform->getInfo<CL_PLATFORM_PROFILE>();
}

std::string Platform::getVendor() const {
	return platform->getInfo<CL_PLATFORM_VENDOR>();
}

std::string Platform::getVersion() const {
	return platform->getInfo<CL_PLATFORM_VERSION>();
}

std::vector<DeviceRef> Platform::getDevices() {
	std::vector<DeviceRef> out;
	std::vector<cl::Device> devices;
	platform->getDevices(CL_DEVICE_TYPE_ALL, &devices);
	for(auto device : devices)
		out.push_back(new Device(this, &device));
	return out;
}

std::vector<PlatformRef> Platform::get() {
	std::vector<PlatformRef> out;
	std::vector<cl::Platform> platforms;
	cl::Platform::get(&platforms);
	for(auto pf : platforms)
		out.push_back(new Platform(&pf));
	return out;
}

} /* namespace CL */
} /* namespace Rendering */
#endif /* RENDERING_HAS_LIB_OPENCL */

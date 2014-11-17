/*
 * Context.cpp
 *
 *  Created on: Nov 11, 2014
 *      Author: sascha
 */

#include "Context.h"

#include <Util/Utils.h>
#include <Util/Macros.h>

namespace Rendering {
namespace CL {

std::vector<cl_context_properties> getContextProperties(const cl::Platform& platform, bool shareGLContext) {
	if(shareGLContext) {
		#if defined (__APPLE__) || defined(MACOSX)
			return {
				CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE, reinterpret_cast<cl_context_properties>(Util::Utils::getCurrentContext()),
				CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties>(platform()),
				0
			};
		#else
			#if defined WIN32 // Win32
				return {
					CL_GL_CONTEXT_KHR, (cl_context_properties)Util::Utils::getCurrentContext(),
					CL_WGL_HDC_KHR, (cl_context_properties)Util::Utils::getCurrentDisplay(),
					CL_CONTEXT_PLATFORM, (cl_context_properties)(platform()),
					0
				};
			#else
				return {
					CL_GL_CONTEXT_KHR, (cl_context_properties)Util::Utils::getCurrentContext(),
					CL_GLX_DISPLAY_KHR, (cl_context_properties)Util::Utils::getCurrentDisplay(),
					CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties>(platform()),
					0
				};
			#endif
		#endif
	} else {
		return {CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties>(platform()), 0};
	}
}

Context::Context(const Platform& platform, uint32_t device_type, bool shareGLContext /*= false*/) {
	cl_int err;
	auto cprops = getContextProperties(platform.platform, shareGLContext);
	context = cl::Context(static_cast<cl_device_type>(device_type), cprops.data(), nullptr, nullptr, &err);
	if(err != CL_SUCCESS)
		WARN("Could not create context (" + std::to_string(err) + ")");
	FAIL_IF(err != CL_SUCCESS);
}

Context::Context(const Platform& platform, const std::vector<Device>& devices, bool shareGLContext /*= false*/) {
	cl_int err;
	auto cprops = getContextProperties(platform.platform, shareGLContext);
	std::vector<cl::Device> cl_devices;
	for(auto device : devices)
		cl_devices.push_back(device.device);
	context = cl::Context(cl_devices, cprops.data(), nullptr, nullptr, &err);
	FAIL_IF(err != CL_SUCCESS);
}

std::vector<Device> Context::getDevices() const {
	std::vector<Device> out;
	std::vector<cl::Device> devices = context.getInfo<CL_CONTEXT_DEVICES>();
	for(auto device : devices)
		out.push_back(device);
	return out;
}


} /* namespace CL */
} /* namespace Rendering */

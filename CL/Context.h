/*
	This file is part of the Rendering library.
	Copyright (C) 2014 Sascha Brandt <myeti@mail.upb.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef RENDERING_HAS_LIB_OPENCL
#ifndef RENDERING_CL_CONTEXT_H_
#define RENDERING_CL_CONTEXT_H_

#include "CLUtils.h"

#include <Util/ReferenceCounter.h>

#include <vector>
#include <memory>

namespace cl {
class Context;
}

namespace Rendering {
namespace CL {
class Platform;
class Device;

class Context : public Util::ReferenceCounter<Context> {
public:
	Context(uint32_t device_type, bool shareGLContext = false);
	Context(Platform* platform, uint32_t device_type, bool shareGLContext = false);
	Context(Platform* platform, const std::vector<DeviceRef>& devices, bool shareGLContext = false);
	Context(Platform* platform, Device* device, bool shareGLContext = false);
	~Context();
	Context(const Context& context);
//	Context(Context&& context);
//	Context& operator=(Context&&);

	std::vector<intptr_t> getProperties() const;

	std::vector<DeviceRef> getDevices();

	Platform* getPlatform() const { return platform.get(); }

	bool isUsingGLInterop() const { return glInterop; };

	cl::Context* _internal() const { return context.get(); }
private:
	std::unique_ptr<cl::Context> context;
	PlatformRef platform;
	std::vector<DeviceRef> devices;
	bool glInterop;
};

} /* namespace CL */
} /* namespace Rendering */

#endif /* RENDERING_CL_CONTEXT_H_ */
#endif /* RENDERING_HAS_LIB_OPENCL */

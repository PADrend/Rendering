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

#include <vector>
#include <memory>

namespace cl {
class Context;
}

namespace Rendering {
namespace CL {
class Platform;
class Device;

class Context {
public:
	Context() {};
	Context(Platform* platform, uint32_t device_type, bool shareGLContext = false);
	Context(Platform* platform, const std::vector<Device*>& devices, bool shareGLContext = false);
	Context(Platform* platform, Device* device, bool shareGLContext = false);
	virtual ~Context() = default;

	std::vector<Device*> getDevices() const;

	cl::Context* _internal() const { return context.get(); };
protected:
	std::unique_ptr<cl::Context> context;
};

} /* namespace CL */
} /* namespace Rendering */

#endif /* RENDERING_CL_CONTEXT_H_ */
#endif /* RENDERING_HAS_LIB_OPENCL */

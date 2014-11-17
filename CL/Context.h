/*
 * Context.h
 *
 *  Created on: Nov 11, 2014
 *      Author: sascha
 */

#ifdef RENDERING_HAS_LIB_OPENCL
#ifndef CONTEXT_H_
#define CONTEXT_H_

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
	Context(Platform* platform, uint32_t device_type, bool shareGLContext = false);
	Context(Platform* platform, const std::vector<Device*>& devices, bool shareGLContext = false);
	virtual ~Context() = default;

	std::vector<Device*> getDevices() const;

	cl::Context* _internal() const { return context.get(); };
private:
	std::unique_ptr<cl::Context> context;
};

} /* namespace CL */
} /* namespace Rendering */

#endif /* CONTEXT_H_ */
#endif /* RENDERING_HAS_LIB_OPENCL */

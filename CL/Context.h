/*
 * Context.h
 *
 *  Created on: Nov 11, 2014
 *      Author: sascha
 */

#ifndef CONTEXT_H_
#define CONTEXT_H_

#include "Platform.h"
#include "Device.h"

#include <CL/cl.hpp>

#include <vector>

namespace Rendering {
namespace CL {

class Context {
public:
	Context(const Platform& platform, uint32_t device_type, bool shareGLContext = false);
	Context(const Platform& platform, const std::vector<Device>& devices, bool shareGLContext = false);
	virtual ~Context() = default;

	std::vector<Device> getDevices() const;

	cl::Context context;
};

} /* namespace CL */
} /* namespace Rendering */

#endif /* CONTEXT_H_ */

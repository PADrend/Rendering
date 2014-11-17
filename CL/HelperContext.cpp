/*
 This file is part of the Rendering library.
 Copyright (C) 2014 Sascha Brandt <myeti@mail.upb.de>

 This library is subject to the terms of the Mozilla Public License, v. 2.0.
 You should have received a copy of the MPL along with this library; see the
 file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef RENDERING_HAS_LIB_OPENCL
#include "HelperContext.h"

#include "Context.h"
#include "Device.h"
#include "Platform.h"
#include "CommandQueue.h"
#include "CLUtils.h"
#include "Event.h"
#include "Kernel.h"
#include "Program.h"
#include "Memory/Buffer.h"
#include "Memory/BufferGL.h"

#include <Util/Macros.h>

#include <CL/cl.hpp>

#include <iostream>

namespace Rendering {
namespace CL {



HelperContext::HelperContext(uint32_t device_type, bool glShare) {
	Platform* pf = nullptr;
	Device* dev = nullptr;
	getFirstPlatformAndDeviceFor(device_type, pf, dev);
	FAIL_IF(pf == nullptr || dev == nullptr);
	platform.reset(pf);
	device.reset(dev);

	init(pf, {dev}, glShare);

	queue.reset(new CommandQueue(this, dev, false, false));
}

void HelperContext::setProgram(const std::string& source) {
	program.reset(new Program(this, source));
}

bool HelperContext::buildProgram() {
	if(!program->build({device.get()}, "")) {
		WARN("Failed to build program.");
		std::cerr << "Build Status: " << program->getBuildStatus(device.get()) << std::endl;
		std::cerr << "Build Options:\t" << program->getBuildOptions(device.get()) << std::endl;
		std::cerr << "Build Log:\t " << program->getBuildLog(device.get()) << std::endl;
		return false;
	}
	return true;
}

Kernel* HelperContext::getKernel(const std::string& name) {
	return new Kernel(program.get(), name);
}

bool HelperContext::execute(Kernel* kernel, const RangeND_t& offset, const RangeND_t& global, const RangeND_t& local) {
	return queue->execute(kernel, offset, global, local);
}

bool HelperContext::read(Buffer* buffer, size_t offset, size_t size, void* ptr) {
	return queue->read(buffer, offset, size, ptr);
}

bool HelperContext::write(Buffer* buffer, size_t offset, size_t size, void* ptr) {
	return queue->write(buffer, offset, size, ptr);
}

bool HelperContext::acquireGLObjects(const std::vector<Buffer*>& buffers) {
	return queue->acquireGLObjects(buffers);
}

bool HelperContext::releaseGLObjects(const std::vector<Buffer*>& buffers) {
	return queue->releaseGLObjects(buffers);
}

void HelperContext::finish() {
	queue->finish();
}

} /* namespace CL */
} /* namespace Rendering */


#endif /* RENDERING_HAS_LIB_OPENCL */

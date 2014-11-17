/*
 This file is part of the Rendering library.
 Copyright (C) 2014 Sascha Brandt <myeti@mail.upb.de>

 This library is subject to the terms of the Mozilla Public License, v. 2.0.
 You should have received a copy of the MPL along with this library; see the
 file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef RENDERING_HAS_LIB_OPENCL
#ifndef Rendering_CL_HELPERCONTEXT_H_
#define Rendering_CL_HELPERCONTEXT_H_

#include "Context.h"
#include "CommandQueue.h"

#include <memory>

namespace Rendering {
namespace CL {
class Platform;
class Device;
class Program;
class Kernel;
class Buffer;

class HelperContext : public Context {
public:
	HelperContext(uint32_t device_type, bool glShare = false);
	virtual ~HelperContext() = default;

	void setProgram(const std::string& source);
	bool buildProgram();

	Kernel* getKernel(const std::string& name);

	bool execute(Kernel* kernel, const RangeND_t& offset, const RangeND_t& global, const RangeND_t& local);

	bool read(Buffer* buffer, size_t offset, size_t size, void* ptr);
	bool write(Buffer* buffer, size_t offset, size_t size, void* ptr);

	bool acquireGLObjects(const std::vector<Buffer*>& buffers);
	bool releaseGLObjects(const std::vector<Buffer*>& buffers);

	void finish();
private:
	std::unique_ptr<Platform> platform;
	std::unique_ptr<Device> device;
	std::unique_ptr<CommandQueue> queue;
	std::unique_ptr<Program> program;
};

} /* namespace CL */
} /* namespace Rendering */

#endif /* Rendering_CL_HELPERCONTEXT_H_ */
#endif /* RENDERING_HAS_LIB_OPENCL */

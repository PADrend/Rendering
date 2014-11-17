/*
	This file is part of the Rendering library.
	Copyright (C) 2014 Sascha Brandt <myeti@mail.upb.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef RENDERING_HAS_LIB_OPENCL
#ifndef RENDERING_CL_COMMANDQUEUE_H_
#define RENDERING_CL_COMMANDQUEUE_H_


#include <array>
#include <vector>
#include <memory>

namespace cl {
class CommandQueue;
}

namespace Rendering {
namespace CL {
class Device;
class Context;
class Buffer;
class Kernel;
class Event;

class RangeND_t {
public:
	template<typename... Args>
	RangeND_t(Args&&... args) : dim(sizeof...(Args)), range({static_cast<size_t>(args)...}) {}
	const size_t dim;
	const std::array<size_t, 3> range;
};

class CommandQueue {
public:
	CommandQueue(Context* context, Device* device, bool outOfOrderExec = false, bool profiling = false);
	virtual ~CommandQueue() = default;

	bool read(Buffer* buffer, size_t offset, size_t size, void* ptr, bool blocking = false, Event* event = nullptr);
	bool write(Buffer* buffer, size_t offset, size_t size, void* ptr, bool blocking = false, Event* event = nullptr);

	bool execute(Kernel* kernel, const RangeND_t& offset, const RangeND_t& global, const RangeND_t& local, Event* event = nullptr);

	bool acquireGLObjects(Buffer* buffer, Event* event = nullptr);
	bool acquireGLObjects(const std::vector<Buffer*>& buffers, Event* event = nullptr);
	bool releaseGLObjects(const std::vector<Buffer*>& buffers, Event* event = nullptr);
	bool releaseGLObjects(Buffer* buffer, Event* event = nullptr);

	void finish();
private:
	std::unique_ptr<cl::CommandQueue> queue;
};

} /* namespace CL */
} /* namespace Rendering */

#endif /* RENDERING_CL_COMMANDQUEUE_H_ */
#endif /* RENDERING_HAS_LIB_OPENCL */

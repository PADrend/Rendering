/*
 * CommandQueue.h
 *
 *  Created on: Nov 13, 2014
 *      Author: sascha
 */

#ifdef RENDERING_HAS_LIB_OPENCL
#ifndef COMMANDQUEUE_H_
#define COMMANDQUEUE_H_


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

	bool execute(Kernel* kernel, RangeND_t offset, RangeND_t global, RangeND_t local, Event* event = nullptr);

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

#endif /* COMMANDQUEUE_H_ */
#endif /* RENDERING_HAS_LIB_OPENCL */

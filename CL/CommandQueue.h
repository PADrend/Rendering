/*
 * CommandQueue.h
 *
 *  Created on: Nov 13, 2014
 *      Author: sascha
 */

#ifndef COMMANDQUEUE_H_
#define COMMANDQUEUE_H_

#include "Event.h"
#include "Context.h"
#include "Device.h"
#include "Buffer.h"
#include "Kernel.h"

#include <CL/cl.hpp>

#include <tuple>
#include <array>
#include <vector>

namespace Rendering {
namespace CL {

class RangeND_t {
public:
	template<typename... Args>
	RangeND_t(Args&&... args) : dim(sizeof...(Args)), range({static_cast<size_t>(args)...}) {}
	const size_t dim;
	const std::array<size_t, 3> range;
};

class CommandQueue {
public:
	CommandQueue(const Context& context, const Device& device, bool outOfOrderExec = false, bool profiling = false);
	virtual ~CommandQueue() = default;

	bool read(const Buffer& buffer, size_t offset, size_t size, void* ptr, bool blocking = false, Event* event = nullptr);
	bool write(const Buffer& buffer, size_t offset, size_t size, void* ptr, bool blocking = false, Event* event = nullptr);

	bool execute(const Kernel& kernel, RangeND_t offset, RangeND_t global, RangeND_t local, Event* event = nullptr);

	bool acquireGLObjects(const Buffer& buffer, Event* event = nullptr);
	bool acquireGLObjects(const std::vector<Buffer>& buffers, Event* event = nullptr);
	bool releaseGLObjects(const std::vector<Buffer>& buffers, Event* event = nullptr);
	bool releaseGLObjects(const Buffer& buffer, Event* event = nullptr);

	void finish();

	cl::CommandQueue queue;
};

} /* namespace CL */
} /* namespace Rendering */

#endif /* COMMANDQUEUE_H_ */

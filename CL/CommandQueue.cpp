/*
 * CommandQueue.cpp
 *
 *  Created on: Nov 13, 2014
 *      Author: sascha
 */

#include "CommandQueue.h"

#include <Util/Macros.h>

#include <iostream>
namespace Rendering {
namespace CL {

template<uint32_t ...S, typename T>
cl::NDRange _toNDRange(T&& tuple) {
	return cl::NDRange(std::get<S>(tuple)...);
}

cl::NDRange toNDRange(const RangeND_t& range) {
	switch(range.dim) {
	case 1:
		return _toNDRange<0>(range.range); break;
	case 2:
		return _toNDRange<0,1>(range.range); break;
	case 3:
		return _toNDRange<0,1,2>(range.range); break;
	}
	return cl::NullRange;
}

CommandQueue::CommandQueue(const Context& context, const Device& device, bool outOfOrderExec /*= false*/, bool profiling /*= false*/) {
	cl_command_queue_properties prop = outOfOrderExec ? CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE : 0;
	if(profiling) prop |= CL_QUEUE_PROFILING_ENABLE;
	cl_int err;
	queue = cl::CommandQueue(context.context, device.device, prop, &err);
	FAIL_IF(err != CL_SUCCESS);
}

bool CommandQueue::read(const Buffer& buffer, size_t offset, size_t size, void* ptr, bool blocking /*= false*/, Event* event /*= nullptr*/) {
	cl_int err = queue.enqueueReadBuffer(buffer.mem, blocking ? CL_TRUE : CL_FALSE, offset, size, ptr, nullptr, event ? &event->event : nullptr);
	if(err != CL_SUCCESS)
		WARN("Could not read buffer (" + std::to_string(err) + ")");
	return err == CL_SUCCESS;
}

bool CommandQueue::write(const Buffer& buffer, size_t offset, size_t size, void* ptr, bool blocking /*= false*/, Event* event /*= nullptr*/) {
	cl_int err = queue.enqueueWriteBuffer(buffer.mem, blocking ? CL_TRUE : CL_FALSE, offset, size, ptr, nullptr, event ? &event->event : nullptr);
	if(err != CL_SUCCESS)
		WARN("Could not write buffer (" + std::to_string(err) + ")");
	return err == CL_SUCCESS;
}

bool CommandQueue::execute(const Kernel& kernel, RangeND_t offset, RangeND_t global, RangeND_t local, Event* event /*= nullptr*/) {
	cl_int err = queue.enqueueNDRangeKernel(kernel.kernel, toNDRange(offset), toNDRange(global), toNDRange(local), nullptr, event ? &event->event : nullptr);
	if(err != CL_SUCCESS)
		WARN("Could not execute kernel (" + std::to_string(err) + ")");
	return err == CL_SUCCESS;
}

bool CommandQueue::acquireGLObjects(const std::vector<Buffer>& buffers, Event* event /*= nullptr*/) {
	std::vector<cl::Memory> cl_buffers;
	for(auto buf : buffers)
		cl_buffers.push_back(buf.mem);
	cl_int err = queue.enqueueAcquireGLObjects(&cl_buffers, nullptr, event ? &event->event : nullptr);
	if(err != CL_SUCCESS)
		WARN("Could not acquire gl objects (" + std::to_string(err) + ")");
	return err == CL_SUCCESS;
}

bool CommandQueue::acquireGLObjects(const Buffer& buffer, Event* event /*= nullptr*/) {
	std::vector<cl::Memory> cl_buffers = {buffer.mem};
	cl_int err = queue.enqueueAcquireGLObjects(&cl_buffers, nullptr, event ? &event->event : nullptr);
	if(err != CL_SUCCESS)
		WARN("Could not acquire gl objects (" + std::to_string(err) + ")");
	return err == CL_SUCCESS;
}

bool CommandQueue::releaseGLObjects(const std::vector<Buffer>& buffers, Event* event /*= nullptr*/) {
	std::vector<cl::Memory> cl_buffers;
	for(auto buf : buffers)
		cl_buffers.push_back(buf.mem);
	cl_int err = queue.enqueueReleaseGLObjects(&cl_buffers, nullptr, event ? &event->event : nullptr);
	if(err != CL_SUCCESS)
		WARN("Could not release gl objects (" + std::to_string(err) + ")");
	return err == CL_SUCCESS;
}

bool CommandQueue::releaseGLObjects(const Buffer& buffer, Event* event /*= nullptr*/) {
	std::vector<cl::Memory> cl_buffers = {buffer.mem};
	cl_int err = queue.enqueueReleaseGLObjects(&cl_buffers, nullptr, event ? &event->event : nullptr);
	if(err != CL_SUCCESS)
		WARN("Could not release gl objects (" + std::to_string(err) + ")");
	return err == CL_SUCCESS;
}

void CommandQueue::finish() {
	queue.finish();
}


} /* namespace CL */
} /* namespace Rendering */


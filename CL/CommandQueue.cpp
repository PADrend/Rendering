/*
	This file is part of the Rendering library.
	Copyright (C) 2014 Sascha Brandt <myeti@mail.upb.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef RENDERING_HAS_LIB_OPENCL
#include "CommandQueue.h"
#include "Event.h"
#include "Context.h"
#include "Device.h"
#include "Memory/Buffer.h"
#include "Kernel.h"
#include "CLUtils.h"

#include <Util/Macros.h>

#include <CL/cl.hpp>

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

CommandQueue::CommandQueue(Context* context, Device* device, bool outOfOrderExec /*= false*/, bool profiling /*= false*/) {
	cl_command_queue_properties prop = outOfOrderExec ? CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE : 0;
	if(profiling) prop |= CL_QUEUE_PROFILING_ENABLE;
	cl_int err;
	queue.reset(new cl::CommandQueue(*context->_internal(), *device->_internal(), prop, &err));
	FAIL_IF(err != CL_SUCCESS);
}

bool CommandQueue::read(Buffer* buffer, size_t offset, size_t size, void* ptr, bool blocking /*= false*/, Event* event /*= nullptr*/) {
	cl_int err = queue->enqueueReadBuffer(*buffer->_internal(), blocking ? CL_TRUE : CL_FALSE, offset, size, ptr, nullptr, event ? event->_internal() : nullptr);
	if(err != CL_SUCCESS)
		WARN("Could not read buffer (" + getErrorString(err) + ")");
	return err == CL_SUCCESS;
}

bool CommandQueue::write(Buffer* buffer, size_t offset, size_t size, void* ptr, bool blocking /*= false*/, Event* event /*= nullptr*/) {
	cl_int err = queue->enqueueWriteBuffer(*buffer->_internal(), blocking ? CL_TRUE : CL_FALSE, offset, size, ptr, nullptr, event ? event->_internal() : nullptr);
	if(err != CL_SUCCESS)
		WARN("Could not write buffer (" + getErrorString(err) + ")");
	return err == CL_SUCCESS;
}

bool CommandQueue::execute(Kernel* kernel, const RangeND_t& offset, const RangeND_t& global, const RangeND_t& local, Event* event /*= nullptr*/) {
	cl_int err = queue->enqueueNDRangeKernel(*kernel->_internal(), toNDRange(offset), toNDRange(global), toNDRange(local), nullptr, event ? event->_internal() : nullptr);
	if(err != CL_SUCCESS)
		WARN("Could not execute kernel (" + getErrorString(err) + ")");
	return err == CL_SUCCESS;
}

bool CommandQueue::acquireGLObjects(const std::vector<Buffer*>& buffers, Event* event /*= nullptr*/) {
	std::vector<cl::Memory> cl_buffers;
	for(auto buf : buffers)
		cl_buffers.push_back(*buf->_internal());
	cl_int err = queue->enqueueAcquireGLObjects(&cl_buffers, nullptr, event ? event->_internal() : nullptr);
	if(err != CL_SUCCESS)
		WARN("Could not acquire gl objects (" + getErrorString(err) + ")");
	return err == CL_SUCCESS;
}

bool CommandQueue::acquireGLObjects(Buffer* buffer, Event* event /*= nullptr*/) {
	std::vector<cl::Memory> cl_buffers = {*buffer->_internal()};
	cl_int err = queue->enqueueAcquireGLObjects(&cl_buffers, nullptr, event ? event->_internal() : nullptr);
	if(err != CL_SUCCESS)
		WARN("Could not acquire gl objects (" + getErrorString(err) + ")");
	return err == CL_SUCCESS;
}

bool CommandQueue::releaseGLObjects(const std::vector<Buffer*>& buffers, Event* event /*= nullptr*/) {
	std::vector<cl::Memory> cl_buffers;
	for(auto buf : buffers)
		cl_buffers.push_back(*buf->_internal());
	cl_int err = queue->enqueueReleaseGLObjects(&cl_buffers, nullptr, event ? event->_internal() : nullptr);
	if(err != CL_SUCCESS)
		WARN("Could not release gl objects (" + getErrorString(err) + ")");
	return err == CL_SUCCESS;
}

bool CommandQueue::releaseGLObjects(Buffer* buffer, Event* event /*= nullptr*/) {
	std::vector<cl::Memory> cl_buffers = {*buffer->_internal()};
	cl_int err = queue->enqueueReleaseGLObjects(&cl_buffers, nullptr, event ? event->_internal() : nullptr);
	if(err != CL_SUCCESS)
		WARN("Could not release gl objects (" + getErrorString(err) + ")");
	return err == CL_SUCCESS;
}

void CommandQueue::finish() {
	queue->finish();
}


} /* namespace CL */
} /* namespace Rendering */
#endif /* RENDERING_HAS_LIB_OPENCL */

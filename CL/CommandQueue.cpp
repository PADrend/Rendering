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
#include "Memory/Memory.h"
#include "Memory/Buffer.h"
#include "Memory/Image.h"
#include "Kernel.h"
#include "CLUtils.h"

#include <Util/Macros.h>

#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#include <CL/cl.hpp>

#include <iostream>

namespace Rendering {
namespace CL {

template<uint32_t ...S, typename T>
cl::NDRange _toNDRange(T&& tuple) {
	return cl::NDRange(std::get<S>(tuple)...);
}

cl::NDRange toNDRange(const RangeND_t& range) {
	switch (range.dim) {
	case 1:
		return _toNDRange<0>(range.range);
		break;
	case 2:
		return _toNDRange<0, 1>(range.range);
		break;
	case 3:
		return _toNDRange<0, 1, 2>(range.range);
		break;
	}
	return cl::NullRange;
}

cl::size_t<3> toSize_t(const RangeND_t& range) {
	cl::size_t<3> sizes;
	sizes[0] = range.range[0];
	sizes[1] = range.range[1];
	sizes[2] = range.range[2];
	return sizes;
}

CommandQueue::CommandQueue(Context* context, Device* device, bool outOfOrderExec /*= false*/, bool profiling /*= false*/) : context(context), device(device) {
	cl_command_queue_properties prop = outOfOrderExec ? CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE : 0;
	if (profiling)
		prop |= CL_QUEUE_PROFILING_ENABLE;
	cl_int err;
	queue.reset(new cl::CommandQueue(*context->_internal(), *device->_internal(), prop, &err));
	FAIL_IF(err != CL_SUCCESS);
}

CommandQueue::~CommandQueue() = default;

CommandQueue::CommandQueue(const CommandQueue& queue) :
		queue(new cl::CommandQueue(*queue.queue.get())), context(queue.context), device(queue.device) {
}

//CommandQueue::CommandQueue(CommandQueue&& queue) = default;
//
//CommandQueue& CommandQueue::operator=(CommandQueue&&) = default;

bool CommandQueue::readBuffer(Buffer* buffer, bool blocking, size_t offset, size_t size, void* ptr, const EventList_t& waitForEvents, Event* event) {
	std::vector<cl::Event> cl_wait;
	for (auto e : waitForEvents)
		cl_wait.push_back(*e->_internal());
	cl_int err = queue->enqueueReadBuffer(*buffer->_internal<cl::Buffer>(), blocking ? CL_TRUE : CL_FALSE, offset, size, ptr, cl_wait.size() > 0 ? &cl_wait : nullptr, event ? event->_internal() : nullptr);
	if (err != CL_SUCCESS)
		WARN("Could not read buffer (" + getErrorString(err) + ")");
	return err == CL_SUCCESS;
}

bool CommandQueue::writeBuffer(Buffer* buffer, bool blocking, size_t offset, size_t size, void* ptr, const EventList_t& waitForEvents, Event* event) {
	std::vector<cl::Event> cl_wait;
	for (auto e : waitForEvents)
		cl_wait.push_back(*e->_internal());
	cl_int err = queue->enqueueWriteBuffer(*buffer->_internal<cl::Buffer>(), blocking ? CL_TRUE : CL_FALSE, offset, size, ptr, cl_wait.size() > 0 ? &cl_wait : nullptr, event ? event->_internal() : nullptr);
	if (err != CL_SUCCESS)
		WARN("Could not write buffer (" + getErrorString(err) + ")");
	return err == CL_SUCCESS;
}

bool CommandQueue::copyBuffer(Buffer* src, Buffer* dst, size_t srcOffset, size_t dstOffset, size_t size, const EventList_t& waitForEvents, Event* event) {
	std::vector<cl::Event> cl_wait;
	for (auto e : waitForEvents)
		cl_wait.push_back(*e->_internal());
	cl_int err = queue->enqueueCopyBuffer(*src->_internal<cl::Buffer>(), *dst->_internal<cl::Buffer>(), srcOffset, dstOffset, size, cl_wait.size() > 0 ? &cl_wait : nullptr, event ? event->_internal() : nullptr);
	if (err != CL_SUCCESS)
		WARN("Could not copy buffer (" + getErrorString(err) + ")");
	return err == CL_SUCCESS;
}

bool CommandQueue::readBufferRect(Buffer* buffer, bool blocking, const RangeND_t& bufferOffset, const RangeND_t& hostOffset, const RangeND_t& region, void* ptr, const EventList_t& waitForEvents, Event* event, size_t bufferRowPitch, size_t bufferSlicePitch, size_t hostRowPitch, size_t hostSlicePitch) {
	cl::size_t<3> cl_buf_off = toSize_t(bufferOffset);
	cl::size_t<3> cl_host_off = toSize_t(hostOffset);
	cl::size_t<3> cl_region = toSize_t(region);
	for (uint_fast8_t i = 0; i < 3; ++i)
		if (cl_region[i] == 0)
			cl_region[i] = 1;
	std::vector<cl::Event> cl_wait;
	for (auto e : waitForEvents)
		cl_wait.push_back(*e->_internal());
	cl_int err = queue->enqueueReadBufferRect(*buffer->_internal<cl::Buffer>(), blocking ? CL_TRUE : CL_FALSE, cl_buf_off, cl_host_off, cl_region, bufferRowPitch, bufferSlicePitch, hostRowPitch, hostSlicePitch, ptr, cl_wait.size() > 0 ? &cl_wait : nullptr, event ? event->_internal() : nullptr);
	if (err != CL_SUCCESS)
		WARN("Could not read buffer rect (" + getErrorString(err) + ")");
	return err == CL_SUCCESS;
}

bool CommandQueue::writeBufferRect(Buffer* buffer, bool blocking, const RangeND_t& bufferOffset, const RangeND_t& hostOffset, const RangeND_t& region, void* ptr, const EventList_t& waitForEvents, Event* event, size_t bufferRowPitch, size_t bufferSlicePitch, size_t hostRowPitch, size_t hostSlicePitch) {
	cl::size_t<3> cl_buf_off = toSize_t(bufferOffset);
	cl::size_t<3> cl_host_off = toSize_t(hostOffset);
	cl::size_t<3> cl_region = toSize_t(region);
	for (uint_fast8_t i = 0; i < 3; ++i)
		if (cl_region[i] == 0)
			cl_region[i] = 1;
	std::vector<cl::Event> cl_wait;
	for (auto e : waitForEvents)
		cl_wait.push_back(*e->_internal());
	cl_int err = queue->enqueueWriteBufferRect(*buffer->_internal<cl::Buffer>(), blocking ? CL_TRUE : CL_FALSE, cl_buf_off, cl_host_off, cl_region, bufferRowPitch, bufferSlicePitch, hostRowPitch, hostSlicePitch, ptr, cl_wait.size() > 0 ? &cl_wait : nullptr, event ? event->_internal() : nullptr);
	if (err != CL_SUCCESS)
		WARN("Could not write buffer rect (" + getErrorString(err) + ")");
	return err == CL_SUCCESS;
}

bool CommandQueue::copyBufferRect(Buffer* src, Buffer* dst, const RangeND_t& srcOrigin, const RangeND_t& dstOrigin, const RangeND_t& region, const EventList_t& waitForEvents, Event* event, size_t srcRowPitch, size_t srcSlicePitch, size_t dstRowPitch, size_t dstSlicePitch) {
	cl::size_t<3> cl_srcorigin = toSize_t(srcOrigin);
	cl::size_t<3> cl_dstcorigin = toSize_t(dstOrigin);
	cl::size_t<3> cl_region = toSize_t(region);
	for (uint_fast8_t i = 0; i < 3; ++i)
		if (cl_region[i] == 0)
			cl_region[i] = 1;
	std::vector<cl::Event> cl_wait;
	for (auto e : waitForEvents)
		cl_wait.push_back(*e->_internal());
	cl_int err = queue->enqueueCopyBufferRect(*src->_internal<cl::Buffer>(), *dst->_internal<cl::Buffer>(), cl_srcorigin, cl_dstcorigin, cl_region, srcRowPitch, srcSlicePitch, dstRowPitch, dstSlicePitch, cl_wait.size() > 0 ? &cl_wait : nullptr, event ? event->_internal() : nullptr);
	if (err != CL_SUCCESS)
		WARN("Could not copy buffer rect (" + getErrorString(err) + ")");
	return err == CL_SUCCESS;
}

bool CommandQueue::readImage(Image* image, bool blocking, const RangeND_t& origin, const RangeND_t& region, void* ptr, const EventList_t& waitForEvents, Event* event, size_t rowPitch, size_t slicePitch) {
	cl::size_t<3> cl_origin = toSize_t(origin);
	cl::size_t<3> cl_region = toSize_t(region);
	for (uint_fast8_t i = 0; i < 3; ++i)
		if (cl_region[i] == 0)
			cl_region[i] = 1;
	std::vector<cl::Event> cl_wait;
	for (auto e : waitForEvents)
		cl_wait.push_back(*e->_internal());
	cl_int err = queue->enqueueReadImage(*image->_internal<cl::Image>(), blocking ? CL_TRUE : CL_FALSE, cl_origin, cl_region, rowPitch, slicePitch, ptr, cl_wait.size() > 0 ? &cl_wait : nullptr, event ? event->_internal() : nullptr);
	if (err != CL_SUCCESS)
		WARN("Could not read image (" + getErrorString(err) + ")");
	return err == CL_SUCCESS;
}

bool CommandQueue::writeImage(Image* image, bool blocking, const RangeND_t& origin, const RangeND_t& region, void* ptr, const EventList_t& waitForEvents, Event* event, size_t rowPitch, size_t slicePitch) {
	cl::size_t<3> cl_origin = toSize_t(origin);
	cl::size_t<3> cl_region = toSize_t(region);
	for (uint_fast8_t i = 0; i < 3; ++i)
		if (cl_region[i] == 0)
			cl_region[i] = 1;
	std::vector<cl::Event> cl_wait;
	for (auto e : waitForEvents)
		cl_wait.push_back(*e->_internal());
	cl_int err = queue->enqueueWriteImage(*image->_internal<cl::Image>(), blocking ? CL_TRUE : CL_FALSE, cl_origin, cl_region, rowPitch, slicePitch, ptr, cl_wait.size() > 0 ? &cl_wait : nullptr, event ? event->_internal() : nullptr);
	if (err != CL_SUCCESS)
		WARN("Could not write image (" + getErrorString(err) + ")");
	return err == CL_SUCCESS;
}

bool CommandQueue::copyImage(Image* src, Image* dst, const RangeND_t& srcOrigin, const RangeND_t& dstOrigin, const RangeND_t& region, const EventList_t& waitForEvents, Event* event) {
	cl::size_t<3> cl_srcorigin = toSize_t(srcOrigin);
	cl::size_t<3> cl_dstcorigin = toSize_t(dstOrigin);
	cl::size_t<3> cl_region = toSize_t(region);
	for (uint_fast8_t i = 0; i < 3; ++i)
		if (cl_region[i] == 0)
			cl_region[i] = 1;
	std::vector<cl::Event> cl_wait;
	for (auto e : waitForEvents)
		cl_wait.push_back(*e->_internal());
	cl_int err = queue->enqueueCopyImage(*src->_internal<cl::Image>(), *dst->_internal<cl::Image>(), cl_srcorigin, cl_dstcorigin, cl_region, cl_wait.size() > 0 ? &cl_wait : nullptr, event ? event->_internal() : nullptr);
	if (err != CL_SUCCESS)
		WARN("Could not copy image (" + getErrorString(err) + ")");
	return err == CL_SUCCESS;
}

bool CommandQueue::copyImageToBuffer(Image* src, Buffer* dst, const RangeND_t& srcOrigin, const RangeND_t& region, size_t dstOffset, const EventList_t& waitForEvents, Event* event) {
	cl::size_t<3> cl_srcorigin = toSize_t(srcOrigin);
	cl::size_t<3> cl_region = toSize_t(region);
	for (uint_fast8_t i = 0; i < 3; ++i)
		if (cl_region[i] == 0)
			cl_region[i] = 1;
	std::vector<cl::Event> cl_wait;
	for (auto e : waitForEvents)
		cl_wait.push_back(*e->_internal());
	cl_int err = queue->enqueueCopyImageToBuffer(*src->_internal<cl::Image>(), *dst->_internal<cl::Buffer>(), cl_srcorigin, cl_region, dstOffset, cl_wait.size() > 0 ? &cl_wait : nullptr, event ? event->_internal() : nullptr);
	if (err != CL_SUCCESS)
		WARN("Could not copy image to buffer (" + getErrorString(err) + ")");
	return err == CL_SUCCESS;
}

bool CommandQueue::copyBufferToImage(Buffer* src, Image* dst, size_t srcOffset, const RangeND_t& dstOrigin, const RangeND_t& region, const EventList_t& waitForEvents, Event* event) {
	cl::size_t<3> cl_dstcorigin = toSize_t(dstOrigin);
	cl::size_t<3> cl_region = toSize_t(region);
	for (uint_fast8_t i = 0; i < 3; ++i)
		if (cl_region[i] == 0)
			cl_region[i] = 1;
	std::vector<cl::Event> cl_wait;
	for (auto e : waitForEvents)
		cl_wait.push_back(*e->_internal());
	cl_int err = queue->enqueueCopyBufferToImage(*src->_internal<cl::Buffer>(), *dst->_internal<cl::Image>(), srcOffset, cl_dstcorigin, cl_region, cl_wait.size() > 0 ? &cl_wait : nullptr, event ? event->_internal() : nullptr);
	if (err != CL_SUCCESS)
		WARN("Could not copy buffer to image (" + getErrorString(err) + ")");
	return err == CL_SUCCESS;
}

void* CommandQueue::mapBuffer(Buffer* buffer, bool blocking, ReadWrite_t readWrite, size_t offset, size_t size, const EventList_t& waitForEvents, Event* event) {
	std::vector<cl::Event> cl_wait;
	for (auto e : waitForEvents)
		cl_wait.push_back(*e->_internal());
	cl_mem_flags flags = readWrite == ReadWrite_t::ReadWrite ? CL_MAP_READ | CL_MAP_WRITE : (readWrite == ReadWrite_t::WriteOnly ? CL_MAP_WRITE : CL_MAP_READ);
	cl_int err;
	void* ptr = queue->enqueueMapBuffer(*buffer->_internal<cl::Buffer>(), blocking ? CL_TRUE : CL_FALSE, flags, offset, size, cl_wait.size() > 0 ? &cl_wait : nullptr, event ? event->_internal() : nullptr, &err);
	if (err != CL_SUCCESS) {
		WARN("Could not map buffer (" + getErrorString(err) + ")");
		ptr = nullptr;
	}
	return ptr;
}

MappedImage CommandQueue::mapImage(Image* image, bool blocking, ReadWrite_t readWrite, const RangeND_t& origin, const RangeND_t& region, const EventList_t& waitForEvents, Event* event) {
	std::vector<cl::Event> cl_wait;
	for (auto e : waitForEvents)
		cl_wait.push_back(*e->_internal());
	cl_mem_flags flags = readWrite == ReadWrite_t::ReadWrite ? CL_MAP_READ | CL_MAP_WRITE : (readWrite == ReadWrite_t::WriteOnly ? CL_MAP_WRITE : CL_MAP_READ);
	cl::size_t<3> cl_origin = toSize_t(origin);
	cl::size_t<3> cl_region = toSize_t(region);
	MappedImage out{nullptr, 0, 0};
	cl_int err;
	out.ptr = queue->enqueueMapImage(*image->_internal<cl::Image>(), blocking ? CL_TRUE : CL_FALSE, flags, cl_origin, cl_region, &out.rowPitch, &out.slicePitch, cl_wait.size() > 0 ? &cl_wait : nullptr, event ? event->_internal() : nullptr, &err);
	if (err != CL_SUCCESS)
		WARN("Could not map image (" + getErrorString(err) + ")");
	return out;
}

bool CommandQueue::unmapMemory(Memory* memory, void* mappedPtr, const EventList_t& waitForEvents, Event* event) {
	std::vector<cl::Event> cl_wait;
	for (auto e : waitForEvents)
		cl_wait.push_back(*e->_internal());
	cl_int err = queue->enqueueUnmapMemObject(*memory->_internal(), mappedPtr, cl_wait.size() > 0 ? &cl_wait : nullptr, event ? event->_internal() : nullptr);
	if (err != CL_SUCCESS)
		WARN("Could not unmap memory (" + getErrorString(err) + ")");
	return err == CL_SUCCESS;
}

bool CommandQueue::execute(Kernel* kernel, const RangeND_t& offset, const RangeND_t& global, const RangeND_t& local, const std::vector<Event*>& waitForEvents, Event* event) {
	std::vector<cl::Event> cl_wait;
	for (auto e : waitForEvents)
		cl_wait.push_back(*e->_internal());
	cl_int err = queue->enqueueNDRangeKernel(*kernel->_internal(), toNDRange(offset), toNDRange(global), toNDRange(local), cl_wait.size() > 0 ? &cl_wait : nullptr, event ? event->_internal() : nullptr);
	if (err != CL_SUCCESS)
		WARN("Could not execute kernel (" + getErrorString(err) + "[" + std::to_string(err) + "])");
	return err == CL_SUCCESS;
}

bool CommandQueue::execute(Kernel* kernel, const EventList_t& waitForEvents, Event* event) {
	std::vector<cl::Event> cl_wait;
	for (auto e : waitForEvents)
		cl_wait.push_back(*e->_internal());
	cl_int err = queue->enqueueTask(*kernel->_internal(), cl_wait.size() > 0 ? &cl_wait : nullptr, event ? event->_internal() : nullptr);
	if (err != CL_SUCCESS)
		WARN("Could not execute kernel (" + getErrorString(err) + ")");
	return err == CL_SUCCESS;
}

bool CommandQueue::execute(std::function<void()> kernel, const EventList_t& waitForEvents, Event* event) {
	struct fnWrapper{
		std::function<void()> function;
		static void kernel(void * userData) {
			fnWrapper* wrapper = static_cast<fnWrapper*>(userData);
			wrapper->function();
		}
	};
	std::vector<cl::Event> cl_wait;
	for (auto e : waitForEvents)
		cl_wait.push_back(*e->_internal());
	fnWrapper wrapper{kernel};
	cl_int err = queue->enqueueNativeKernel(fnWrapper::kernel, std::make_pair(&wrapper, sizeof(fnWrapper)), nullptr, nullptr, cl_wait.size() > 0 ? &cl_wait : nullptr, event ? event->_internal() : nullptr);
	if (err != CL_SUCCESS)
		WARN("Could not execute kernel (" + getErrorString(err) + ")");
	return err == CL_SUCCESS;
}

bool CommandQueue::acquireGLObjects(const std::vector<Memory*>& buffers, const EventList_t& waitForEvents, Event* event) {
	std::vector<cl::Event> cl_wait;
	for (auto e : waitForEvents)
		cl_wait.push_back(*e->_internal());
	std::vector<cl::Memory> cl_buffers;
	for (auto buf : buffers)
		cl_buffers.push_back(*buf->_internal());
	cl_int err = queue->enqueueAcquireGLObjects(&cl_buffers, cl_wait.size() > 0 ? &cl_wait : nullptr, event ? event->_internal() : nullptr);
	if (err != CL_SUCCESS)
		WARN("Could not acquire gl objects (" + getErrorString(err) + ")");
	return err == CL_SUCCESS;
}

bool CommandQueue::releaseGLObjects(const std::vector<Memory*>& buffers, const EventList_t& waitForEvents, Event* event) {
	std::vector<cl::Event> cl_wait;
	for (auto e : waitForEvents)
		cl_wait.push_back(*e->_internal());
	std::vector<cl::Memory> cl_buffers;
	for (auto buf : buffers)
		cl_buffers.push_back(*buf->_internal());
	cl_int err = queue->enqueueReleaseGLObjects(&cl_buffers, cl_wait.size() > 0 ? &cl_wait : nullptr, event ? event->_internal() : nullptr);
	if (err != CL_SUCCESS)
		WARN("Could not release gl objects (" + getErrorString(err) + ")");
	return err == CL_SUCCESS;
}

void CommandQueue::marker(Event* event) {
#if defined(CL_VERSION_1_2) && !defined(CL_USE_DEPRECATED_OPENCL_1_1_APIS)
	queue->enqueueMarkerWithWaitList(nullptr, event ? event->_internal() : nullptr);
#else
	queue->enqueueMarker(event != nullptr ? event->_internal() : nullptr);
#endif
}

void CommandQueue::waitForEvents(const EventList_t& waitForEvents) {
	std::vector<cl::Event> cl_wait;
	for (auto e : waitForEvents)
		cl_wait.push_back(*e->_internal());
#if defined(CL_VERSION_1_2) && !defined(CL_USE_DEPRECATED_OPENCL_1_1_APIS)
	queue->enqueueMarkerWithWaitList(&cl_wait, nullptr);
#else
	queue->enqueueWaitForEvents(cl_wait);
#endif
}

void CommandQueue::barrier() {
#if defined(CL_VERSION_1_2) && !defined(CL_USE_DEPRECATED_OPENCL_1_1_APIS)
	queue->enqueueBarrierWithWaitList(nullptr, nullptr);
#else
	queue->enqueueBarrier();
#endif
}

void CommandQueue::finish() {
	queue->finish();
}

void CommandQueue::flush() {
	queue->flush();
}

} /* namespace CL */
} /* namespace Rendering */
#endif /* RENDERING_HAS_LIB_OPENCL */

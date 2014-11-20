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
#include <functional>

namespace cl {
class CommandQueue;
}

namespace Rendering {
namespace CL {
class Device;
class Context;
class Buffer;
class Image;
class Memory;
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
	typedef std::vector<Event*> EventList_t;
	enum MapFlags_t { Read, Write };

	CommandQueue(Context* context, Device* device, bool outOfOrderExec = false, bool profiling = false);
	~CommandQueue();
	CommandQueue(const CommandQueue& queue);
	CommandQueue(CommandQueue&& queue);
	CommandQueue& operator=(CommandQueue&&);

	bool readBuffer(Buffer* buffer, bool blocking, size_t offset, size_t size, void* ptr, const EventList_t& waitForEvents = EventList_t(), Event* event = nullptr);
	bool writeBuffer(Buffer* buffer, bool blocking, size_t offset, size_t size, void* ptr, const EventList_t& waitForEvents = EventList_t(), Event* event = nullptr);
	bool copyBuffer(Buffer* src, Buffer* dst, size_t srcOffset, size_t dstOffset, size_t size, const EventList_t& waitForEvents = EventList_t(), Event* event = nullptr);

	bool readBufferRect(Buffer* buffer, bool blocking, const RangeND_t& bufferOffset, const RangeND_t& hostOffset, const RangeND_t& region, void* ptr, const EventList_t& waitForEvents = EventList_t(), Event* event = nullptr, size_t bufferRowPitch = 0, size_t bufferSlicePitch = 0, size_t hostRowPitch = 0, size_t hostSlicePitch = 0);
	bool writeBufferRect(Buffer* buffer, bool blocking, const RangeND_t& bufferOffset, const RangeND_t& hostOffset, const RangeND_t& region, void* ptr, const EventList_t& waitForEvents = EventList_t(), Event* event = nullptr, size_t bufferRowPitch = 0, size_t bufferSlicePitch = 0, size_t hostRowPitch = 0, size_t hostSlicePitch = 0);
	bool copyBufferRect(Buffer* src, Buffer* dst, const RangeND_t& srcOrigin, const RangeND_t& dstOrigin, const RangeND_t& region, const EventList_t& waitForEvents = EventList_t(), Event* event = nullptr, size_t bufferRowPitch = 0, size_t bufferSlicePitch = 0, size_t hostRowPitch = 0, size_t hostSlicePitch = 0);

	bool readImage(Image* image, bool blocking, const RangeND_t& origin, const RangeND_t& region, void* ptr, const EventList_t& waitForEvents = EventList_t(), Event* event = nullptr, size_t rowPitch = 0, size_t slicePitch = 0);
	bool writeImage(Image* image, bool blocking, const RangeND_t& origin, const RangeND_t& region, void* ptr, const EventList_t& waitForEvents = EventList_t(), Event* event = nullptr, size_t rowPitch = 0, size_t slicePitch = 0);
	bool copyImage(Image* src, Image* dst, const RangeND_t& srcOrigin, const RangeND_t& dstOrigin, const RangeND_t& region, const EventList_t& waitForEvents = EventList_t(), Event* event = nullptr);

	bool copyImageToBuffer(Image* src, Buffer* dst, const RangeND_t& srcOrigin, const RangeND_t& region, size_t dstOffset, const EventList_t& waitForEvents = EventList_t(), Event* event = nullptr);
	bool copyBufferToImage(Buffer* src, Image* dst, size_t srcOffset, const RangeND_t& dstOrigin, const RangeND_t& region, const EventList_t& waitForEvents = EventList_t(), Event* event = nullptr);

	void* mapBuffer(Buffer* buffer, bool blocking, MapFlags_t readWrite, size_t offset, size_t size, const EventList_t& waitForEvents = EventList_t(), Event* event = nullptr);
	void* mapImage(Image* buffer, bool blocking, MapFlags_t readWrite, const RangeND_t& origin, const RangeND_t& region, const EventList_t& waitForEvents = EventList_t(), Event* event = nullptr, size_t rowPitch = 0, size_t slicePitch = 0);
	bool unmapMemory(Memory* memory, void* mappedPtr, const EventList_t& waitForEvents, Event* event = nullptr);

	bool execute(Kernel* kernel, const RangeND_t& offset, const RangeND_t& global, const RangeND_t& local, const EventList_t& waitForEvents = EventList_t(), Event* event = nullptr);
	bool execute(Kernel* kernel, const EventList_t& waitForEvents = EventList_t(), Event* event = nullptr);
	bool execute(std::function<void()> kernel, const EventList_t& waitForEvents = EventList_t(), Event* event = nullptr);

	bool acquireGLObjects(const std::vector<Memory*>& buffers, const EventList_t& waitForEvents = EventList_t(), Event* event = nullptr);
	bool releaseGLObjects(const std::vector<Memory*>& buffers, const EventList_t& waitForEvents = EventList_t(), Event* event = nullptr);

	void marker(Event* event = nullptr);
	void waitForEvents(const EventList_t& waitForEvents);

	void barrier();
	void finish();
	void flush();
private:
	std::unique_ptr<cl::CommandQueue> queue;
};

} /* namespace CL */
} /* namespace Rendering */

#endif /* RENDERING_CL_COMMANDQUEUE_H_ */
#endif /* RENDERING_HAS_LIB_OPENCL */

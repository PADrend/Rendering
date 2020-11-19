/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_CONTEXT_RENDERTHREAD_H_
#define RENDERING_CONTEXT_RENDERTHREAD_H_

#include <Util/ReferenceCounter.h>

#include <mutex>
#include <thread>
#include <deque>
#include <atomic>
#include <condition_variable>
#include <functional>

namespace Rendering {
class Device;
using DeviceRef = Util::Reference<Device>;
class CommandBuffer;
using CommandBufferRef = Util::Reference<CommandBuffer>;

class RenderThread : public Util::ReferenceCounter<RenderThread> {
public:
	using Task = std::function<void ()>;
	using Ref = Util::Reference<RenderThread>;
	RENDERINGAPI static const Ref& get();
	RenderThread(RenderThread &&) = default;
	RenderThread(const RenderThread &) = delete;
	RENDERINGAPI ~RenderThread();

	static uint64_t addTask(const Task& task) { return get()->_addTask(task); }
	static void sync(uint64_t taskId) { get()->_sync(taskId); }
	static uint64_t getProcessed() { return get()->processedCount; }

	static bool isInRenderThread() { return std::this_thread::get_id() == get()->worker.get_id(); }

private:
	RenderThread();
	void run();
	
	uint64_t _addTask(const Task& task);
	void _sync(uint64_t taskId);

	std::thread worker;
	std::atomic<bool> running;

	std::mutex queueMutex;
	std::condition_variable queueCond;
	std::atomic<uint64_t> submittedCount;

	std::mutex processedMutex;
	std::condition_variable processedCond;	
	std::atomic<uint64_t> processedCount;
	
	std::deque<Task> queue;
};

} /* Rendering */

#endif /* end of include guard: RENDERING_CONTEXT_RENDERTHREAD_H_ */
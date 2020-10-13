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
#include <queue>
#include <atomic>
#include <condition_variable>

namespace Rendering {
class Device;
using DeviceRef = Util::Reference<Device>;
class CommandBuffer;
using CommandBufferRef = Util::Reference<CommandBuffer>;

class RenderThread : public Util::ReferenceCounter<RenderThread> {
public:
	using Ref = Util::Reference<RenderThread>;
	static Ref create(const DeviceRef& device);
	RenderThread(RenderThread &&) = default;
	RenderThread(const RenderThread &) = delete;
	~RenderThread();
	
	void compileAndSubmit(const CommandBufferRef& cmd);
private:
	RenderThread(const DeviceRef& device);
	void run();

	DeviceRef device;
	std::thread worker;
	std::queue<CommandBufferRef> queue;
	std::mutex mutex;
	std::condition_variable condition;
	std::atomic<bool> running;
};

} /* Rendering */

#endif /* end of include guard: RENDERING_CONTEXT_RENDERTHREAD_H_ */
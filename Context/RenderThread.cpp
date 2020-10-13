/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "RenderThread.h"
#include "../Core/Device.h"
#include "../Core/CommandBuffer.h"

#include <functional>

namespace Rendering {

//---------------

RenderThread::Ref RenderThread::create(const DeviceRef& device) {
	Ref obj = new RenderThread(device);
	return obj;
}

//---------------

RenderThread::~RenderThread() {
	running = false;
	condition.notify_all();
	worker.join();
}

//---------------

RenderThread::RenderThread(const DeviceRef& device) : device(device), worker(std::bind(&RenderThread::run, this)) {}

//---------------

void RenderThread::compileAndSubmit(const CommandBufferRef& cmd) {
	std::unique_lock<std::mutex> lock(mutex);
	queue.push(cmd);
	lock.unlock();
	condition.notify_all();
}

//---------------

void RenderThread::run() {
	std::unique_lock<std::mutex> lock(mutex);
	while(running) {
		condition.wait(lock, [this]{ return !queue.empty() || !running; });

		if(!queue.empty()) {
			auto cmd = queue.front();
			queue.pop();
			lock.unlock();

			cmd->submit();

			lock.lock();
		}
	}
}

//---------------

} /* Rendering */
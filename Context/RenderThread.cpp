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

const RenderThread::Ref& RenderThread::get() {
	static Ref thread = new RenderThread();
	return thread;
}

//---------------

RenderThread::~RenderThread() {
	running = false;
	processedCond.notify_all();
	queueCond.notify_all();
	worker.join();
}

//---------------

RenderThread::RenderThread() : worker(std::bind(&RenderThread::run, this)), running(true), submittedCount(0), processedCount(0) {}

//---------------

uint64_t RenderThread::_addTask(const Task& task) {
	std::unique_lock<std::mutex> lock(queueMutex);
	queue.push_back(task);
	uint64_t returnValue = ++submittedCount;
	lock.unlock();
	queueCond.notify_all();
	return returnValue;
}

//---------------

void RenderThread::_sync(uint64_t taskId) {
	std::unique_lock<std::mutex> lock(processedMutex);
	processedCond.wait(lock, [&]{ return processedCount >= taskId || !running; });
}

//---------------

void RenderThread::run() {
	std::unique_lock<std::mutex> lock(queueMutex);
	while(running) {
		queueCond.wait(lock, [this]{ return !queue.empty() || !running; });

		if(!queue.empty()) {
			auto task = std::move(queue.front());
			queue.pop_front();
			lock.unlock();
			task();
			processedCount++;
			processedCond.notify_all();
			lock.lock();
		}
	}
}

//---------------

} /* Rendering */
/*
	This file is part of the Rendering library.
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "Fence.h"
#include "Device.h"
#include "Queue.h"

#include <Util/Macros.h>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#include <vector>

namespace Rendering {

//---------------

Fence::Ref Fence::create() {
	return new Fence();
}

//---------------

Fence::~Fence() = default;

//---------------

void Fence::wait() {
	if(fenceQueue.empty())
		return;
	vk::Device device(fenceQueue.front());
	std::vector<vk::Fence> fences(fenceQueue.begin(), fenceQueue.end());
	device.waitForFences(fences, true, std::numeric_limits<uint64_t>::max());
	fenceQueue.clear();
}

//---------------

uint64_t Fence::signal(const QueueRef& queue) {
	WARN_AND_RETURN_IF(!queue || !queue->getApiHandle(), "Cannot signal fence. Invalid command queue.", 0);
	vk::Device vkDevice(queue->getApiHandle());
	vk::Queue vkQueue(queue->getApiHandle());
	++cpuValue;
	FenceHandle fence = FenceHandle::create(vkDevice.createFence({}), vkDevice);
	vkQueue.submit({}, {fence});
	fenceQueue.emplace_back(fence);
	return cpuValue-1;
}

//---------------

uint64_t Fence::getGpuValue() {
	if(fenceQueue.empty())
		return gpuValue;
	vk::Device vkDevice(fenceQueue.front());
	while(!fenceQueue.empty()) {
		if(vkDevice.getFenceStatus({fenceQueue.front()}) == vk::Result::eSuccess ) {
			fenceQueue.pop_front(); // fence is signaled
			++gpuValue;
		} else {
			break;
		}
	}
	return gpuValue;
}

//---------------


} /* Rendering */
/*
	This file is part of the Rendering library.
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "Queue.h"
#include "Device.h"
#include "Swapchain.h"
#include "CommandBuffer.h"
#include "../Shader/Shader.h"
#include "../FBO.h"
#include "../Texture/Texture.h"

#include <Util/Macros.h>
#include <Util/StringUtils.h>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

namespace Rendering {

//-------------

Queue::Queue(const DeviceRef& device, uint32_t familyIndex, uint32_t index) : device(device.get()), familyIndex(familyIndex), index(index) { }

//-------------

Queue::~Queue() { if(device->isDebugModeEnabled()) std::cout << "Destroying Queue " << index << " of family " << familyIndex << "..." << std::endl; }

//-------------

bool Queue::submit(const CommandBufferRef& commands) {
	WARN_AND_RETURN_IF(!commands, "Queue: invalid command buffer.", false);
	WARN_AND_RETURN_IF(!commands->compile(), "Queue: Command buffer is not executable.", false);
	std::unique_lock<std::mutex> lock(submitMutex);
	clearPending();
	vk::Device vkDevice(handle);
	vk::CommandBuffer vkCommandBuffer(commands->getApiHandle());
	FenceHandle fence = FenceHandle::create(vkDevice.createFence({}), vkDevice);
	pendingQueue.emplace_back(PendingEntry{commands, fence});
	vk::Semaphore signalSemaphore(commands->getSignalSemaphore());
	static_cast<vk::Queue>(handle).submit({{
		0, nullptr, nullptr,
		1, &vkCommandBuffer,
		1, &signalSemaphore
	}}, static_cast<vk::Fence>(fence));
	return true;
}

//-------------

bool Queue::submit(const FenceHandle& fence) {
	WARN_AND_RETURN_IF(!fence, "Queue: invalid fence.", false);
	std::unique_lock<std::mutex> lock(submitMutex);
	static_cast<vk::Queue>(handle).submit({}, static_cast<vk::Fence>(fence));
	return true;
}

//-------------

bool Queue::present() {
	if(!supports(QueueFamily::Present)) {
		WARN("Queue: Present is not supported by queue " + Util::StringUtils::toString(index) + " of family " + Util::StringUtils::toString(familyIndex) + ".");
		return false;
	}
	std::unique_lock<std::mutex> lock(submitMutex);
	clearPending();
	auto& swapchain = device->getSwapchain();
	vk::SwapchainKHR vkSwapchain(swapchain->getApiHandle());
	vk::Queue vkQueue(handle);
	auto swapchainIndex = swapchain->getCurrentIndex();
	
	vkQueue.presentKHR({
		0, nullptr,
		1, &vkSwapchain,
		&swapchainIndex
	});
	swapchain->acquireNextIndex();
	return true;
}

//-------------

void Queue::wait() {
	std::unique_lock<std::mutex> lock(submitMutex);
	vk::Device vkDevice(handle);
	std::vector<vk::Fence> fences;
	for(auto& pending : pendingQueue) {
		fences.emplace_back(pending.fence);
	}
	vkDevice.waitForFences(fences, true, std::numeric_limits<uint64_t>::max());
	pendingQueue.clear();
}

//-------------

void Queue::clearPending() {
	vk::Device vkDevice(handle);
	while(!pendingQueue.empty()) {
		vk::Fence fence(pendingQueue.front().fence);
		if(vkDevice.getFenceStatus(fence) == vk::Result::eSuccess ) {
			pendingQueue.pop_front(); // fence is signaled
		} else {
			break;
		}
	}
}


//-------------

CommandBufferHandle Queue::requestCommandBuffer(bool primary, uint32_t threadId) {
	std::unique_lock<std::mutex> lock(poolMutex);
	int32_t key = static_cast<int32_t>(threadId);
	if(!primary)
		key = -(key+1);

	if(!commandPool.hasType(key)) {
		// lazily create new command pool
		vk::Device vkDevice(device->getApiHandle());
		auto pool = CommandPoolHandle::create(vkDevice.createCommandPool({
			vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
			familyIndex
		}), vkDevice);
		commandPool.registerType(key, std::bind(&Queue::createCommandBuffer, this, pool, key >= 0));
	}

	return commandPool.create(key);
}


//-------------

void Queue::freeCommandBuffer(const CommandBufferHandle& bufferHandle, bool primary, uint32_t threadId) {
	std::unique_lock<std::mutex> lock(poolMutex);
	int32_t key = static_cast<int32_t>(threadId);
	if(!primary)
		key = -(key+1);
	commandPool.free(key, bufferHandle);
}

//-------------

bool Queue::init() {
	vk::Device vkDevice(device->getApiHandle());
	vk::PhysicalDevice physicalDevice(device->getApiHandle());
	vk::SurfaceKHR surface(device->getSurface());

	auto queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
	QueueFamily isPresent = physicalDevice.getSurfaceSupportKHR(familyIndex, surface) ? QueueFamily::Present : QueueFamily::None;
	QueueFamily isGraphics = (queueFamilyProperties[familyIndex].queueFlags & vk::QueueFlagBits::eGraphics) ? QueueFamily::Graphics : QueueFamily::None;
	QueueFamily isCompute = (queueFamilyProperties[familyIndex].queueFlags & vk::QueueFlagBits::eCompute) ? QueueFamily::Compute : QueueFamily::None;
	QueueFamily isTransfer = (queueFamilyProperties[familyIndex].queueFlags & vk::QueueFlagBits::eTransfer) ? QueueFamily::Transfer : QueueFamily::None;
	capabilities = isPresent | isGraphics | isCompute | isTransfer;

	handle = QueueHandle::create(vkDevice.getQueue(familyIndex, index), vkDevice);
	return handle.isNotNull();
}

//-------------

CommandBufferHandle Queue::createCommandBuffer(CommandPoolHandle pool, bool primary) {
	vk::Device vkDevice(handle);
	vk::CommandPool vkPool(pool);

	auto buffers = vkDevice.allocateCommandBuffers({
		vkPool,
		primary ? vk::CommandBufferLevel::ePrimary : vk::CommandBufferLevel::eSecondary,
		1u
	});

	if(buffers.empty() || !buffers.front())
		return nullptr;
	
	return CommandBufferHandle::create(buffers.front(), {vkDevice, vkPool});
}

//-------------

} /* Rendering */

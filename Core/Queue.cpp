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

#include <Util/Macros.h>
#include <Util/StringUtils.h>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

namespace Rendering {

//-------------

Queue::Queue(const DeviceRef& device, uint32_t familyIndex, uint32_t index) : device(device.get()), familyIndex(familyIndex), index(index) { }

//-------------

Queue::~Queue() = default;

//-------------

bool Queue::submit(const CommandBufferRef& commands, bool wait) {
	WARN_AND_RETURN_IF(!commands, "Queue: invalid command buffer.", false);
	if(commands->isRecording())
		commands->end();
	WARN_AND_RETURN_IF(!commands->isExecutable(), "Queue: command buffer is not executable.", false);
	clearPending();
	vk::Device vkDevice(handle);
	vk::Queue vkQueue(handle);
	vk::CommandBuffer vkCommandBuffer(commands->getApiHandle());
	FenceHandle fence = FenceHandle::create(vkDevice.createFence({}), vkDevice);
	pendingQueue.emplace_back(PendingEntry{commands, fence});
	vkQueue.submit({{
		0, nullptr, nullptr,
		1, &vkCommandBuffer,
		0, nullptr
	}}, static_cast<vk::Fence>(fence));
	if(wait)
		this->wait();
	return true;
}

//-------------

bool Queue::present() {
	if(!supports(QueueFamily::Present)) {
		WARN("Queue: Present is not supported by queue " + Util::StringUtils::toString(index) + " of family " + Util::StringUtils::toString(familyIndex) + ".");
		return false;
	}
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
	vk::Device vkDevice(handle);
	std::vector<vk::Fence> fences;
	for(auto& pending : pendingQueue) {
		fences.emplace_back(pending.fence);
	}
	vkDevice.waitForFences(fences, true, std::numeric_limits<uint64_t>::max());
	for(auto& pending : pendingQueue) {
		pending.cmd->state = CommandBuffer::State::Executable;
	}
	pendingQueue.clear();
}

//-------------

void Queue::clearPending() {
	vk::Device vkDevice(handle);
	while(!pendingQueue.empty()) {
		vk::Fence fence(pendingQueue.front().fence);
		if(vkDevice.getFenceStatus(fence) == vk::Result::eSuccess ) {
			pendingQueue.front().cmd->state = CommandBuffer::State::Executable;
			pendingQueue.pop_front(); // fence is signaled
		} else {
			break;
		}
	}
}


//-------------

CommandBufferHandle Queue::requestCommandBuffer(bool primary) {
	return commandPool.create(static_cast<uint32_t>(primary ? vk::CommandBufferLevel::ePrimary : vk::CommandBufferLevel::eSecondary));
}


//-------------

void Queue::freeCommandBuffer(const CommandBufferHandle& bufferHandle, bool primary) {
	commandPool.free(static_cast<uint32_t>(primary ? vk::CommandBufferLevel::ePrimary : vk::CommandBufferLevel::eSecondary), bufferHandle);
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
	if(!handle)
		return false;

	commandPoolHandle = CommandPoolHandle::create(vkDevice.createCommandPool({
		vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		familyIndex
	}), vkDevice);

	commandPool.registerType(static_cast<uint32_t>(vk::CommandBufferLevel::ePrimary), std::bind(&Queue::createCommandBuffer, this, true));
	commandPool.registerType(static_cast<uint32_t>(vk::CommandBufferLevel::eSecondary), std::bind(&Queue::createCommandBuffer, this, false));
	
	return commandPoolHandle.isNotNull();
}

//-------------

CommandBufferHandle Queue::createCommandBuffer(bool primary) {
	vk::Device vkDevice(handle);
	vk::CommandPool vkPool(commandPoolHandle);

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

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
#include "CommandPool.h"
#include "../Shader/Shader.h"
#include "../FBO.h"

#include <Util/Macros.h>
#include <Util/StringUtils.h>

#include <vulkan/vulkan.hpp>

namespace Rendering {

//-------------

bool Queue::submit(const CommandBufferRef& commands) {
	WARN_AND_RETURN_IF(!commands || !commands->isExecutable(), "Queue: command buffer is not executable.", false);
	vk::Queue vkQueue(handle);
	vk::CommandBuffer vkCommandBuffer(commands->getApiHandle());
	vkQueue.submit({{
		0, nullptr, nullptr,
		1, &vkCommandBuffer,
		0, nullptr
	}}, {});
	return true;
}

//-------------

bool Queue::present() {
	if(!supports(QueueFamily::Present)) {
		WARN("Queue: Present is not supported by queue " + Util::StringUtils::toString(index) + " of family " + Util::StringUtils::toString(familyIndex) + ".");
		return false;
	}
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

CommandBufferRef Queue::requestCommandBuffer(bool primary) {
	return device->getCommandPool(familyIndex)->requestCommandBuffer(primary);
}

//-------------

const CommandPoolRef& Queue::getCommandPool() const {
	return device->getCommandPool(familyIndex);
}

//-------------

Queue::Queue(const DeviceRef& device, uint32_t familyIndex, uint32_t index) : device(device), familyIndex(familyIndex), index(index) {
	vk::Device vkDevice(device->getApiHandle());
	vk::PhysicalDevice physicalDevice(device->getApiHandle());
	vk::SurfaceKHR surface(device->getSurface());

	handle = QueueHandle::create(vkDevice.getQueue(familyIndex, index), vkDevice);
	auto queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
	QueueFamily isPresent = physicalDevice.getSurfaceSupportKHR(familyIndex, surface) ? QueueFamily::Present : QueueFamily::None;
	QueueFamily isGraphics = (queueFamilyProperties[familyIndex].queueFlags & vk::QueueFlagBits::eGraphics) ? QueueFamily::Graphics : QueueFamily::None;
	QueueFamily isCompute = (queueFamilyProperties[familyIndex].queueFlags & vk::QueueFlagBits::eCompute) ? QueueFamily::Compute : QueueFamily::None;
	QueueFamily isTransfer = (queueFamilyProperties[familyIndex].queueFlags & vk::QueueFlagBits::eTransfer) ? QueueFamily::Transfer : QueueFamily::None;
	capabilities = isPresent | isGraphics | isCompute | isTransfer;
}

//-------------

} /* Rendering */

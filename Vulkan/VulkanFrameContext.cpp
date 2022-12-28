/*
	This file is part of the Platform for Algorithm Development and Rendering (PADrend).
	Web page: http://www.padrend.de/
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	Copyright (C) 2014-2022 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "VulkanFrameContext.h"
#include "VulkanDevice.h"

#include <Util/UI/Window.h>
#include <Util/StringUtils.h>

#define VULKAN_HPP_NO_EXCEPTIONS
#include <nvrhi/nvrhi.h>
#include <nvrhi/validation.h>
#include <nvrhi/vulkan.h>

#include <vector>
#include <queue>
#include <unordered_set>
#include <iostream>

namespace Rendering {
using namespace Util;

//-------------------------

struct VulkanSwapChainImage {
	vk::Image image;
	nvrhi::TextureHandle rhiHandle;
	nvrhi::FramebufferHandle framebuffer;
};

//-------------------------

struct VulkanFrameContext::Internal {
	Internal(const VulkanDeviceHandle& device) : device(device) {}
	VulkanDeviceHandle device;
	
	// window
	vk::SurfaceKHR surface				 = nullptr;
	vk::SwapchainKHR swapChain		 = nullptr;
	vk::Semaphore presentSemaphore = nullptr;
	nvrhi::CommandListHandle barrierCommandList;
	std::vector<VulkanSwapChainImage> swapChainImages;
	uint32_t swapChainIndex = uint32_t(-1);

	std::queue<nvrhi::EventQueryHandle> framesInFlight;
	std::vector<nvrhi::EventQueryHandle> queryPool;
};

//-------------------------

VulkanFrameContext::VulkanFrameContext(const VulkanDeviceHandle& device, const WindowHandle& handle) : data(new Internal(device)), RenderFrameContext(handle) {
}

//-------------------------

VulkanFrameContext::~VulkanFrameContext() {
	destroySwapChain();

	auto vkDevice = data->device->_getVkDevice();
	if (vkDevice && data->presentSemaphore) {
		vkDevice.destroy(data->presentSemaphore);
	}
	data->device = nullptr;

	VulkanInstance::_getVkInstance().destroy(data->surface);
}

//-------------------------

void VulkanFrameContext::refresh() {
	recreateSwapChain();
}

//-------------------------

void VulkanFrameContext::beginFrame() {
	auto vkDevice = data->device->_getVkDevice();
	auto nvDevice = data->device->_getNvDevice();
	const auto result = vkDevice.acquireNextImageKHR(data->swapChain, std::numeric_limits<uint64_t>::max(), data->presentSemaphore, {}, &data->swapChainIndex);
	WARN_AND_RETURN_IF(result != vk::Result::eSuccess, "Cannot begin frame. Failed to aquire next swapchain image.", );
	nvDevice->queueWaitForSemaphore(nvrhi::CommandQueue::Graphics, data->presentSemaphore, 0);
}

//-------------------------

void VulkanFrameContext::endFrame() {
	auto vkDevice = data->device->_getVkDevice();
	auto nvDevice = data->device->_getNvDevice();
	nvDevice->queueSignalSemaphore(nvrhi::CommandQueue::Graphics, data->presentSemaphore, 0);

	data->barrierCommandList->open();
	data->barrierCommandList->close();
	nvDevice->executeCommandList(data->barrierCommandList);

	vk::PresentInfoKHR info = vk::PresentInfoKHR()
		.setWaitSemaphoreCount(1)
		.setPWaitSemaphores(&data->presentSemaphore)
		.setSwapchainCount(1)
		.setPSwapchains(&data->swapChain)
		.setPImageIndices(&data->swapChainIndex);

	auto presentQueue = data->device->_getVkQueue(QueueFamily::Present);
	const vk::Result res = data->device->_getVkQueue(QueueFamily::Present).presentKHR(&info);
	WARN_AND_RETURN_IF(res != vk::Result::eSuccess && res != vk::Result::eErrorOutOfDateKHR, "Failed to present frame.",);

	if (VulkanInstance::isDebugModeEnabled()) {
		auto result = presentQueue.waitIdle();
	} else {
	
		const uint32_t maxFramesInFlight = 2; // TODO: make configurable
		while (data->framesInFlight.size() > maxFramesInFlight) {
				auto query = data->framesInFlight.front();
				data->framesInFlight.pop();

				nvDevice->waitEventQuery(query);

				data->queryPool.push_back(query);
		}

		nvrhi::EventQueryHandle query;
		if (!data->queryPool.empty())
		{
				query = data->queryPool.back();
				data->queryPool.pop_back();
		}
		else
		{
				query = nvDevice->createEventQuery();
		}

		nvDevice->resetEventQuery(query);
		nvDevice->setEventQuery(query, nvrhi::CommandQueue::Graphics);
		data->framesInFlight.push(query);
	}

}

//-------------------------

bool VulkanFrameContext::init() {
	if (!window || !data->device->isWindowRenderingSupported())
		return false;

	// create window surface
	data->surface = {window->createSurface(data->device->_getVkInstance())};
	WARN_AND_RETURN_IF(!data->surface, "Failed to create surface.", false);

	// create swapchain
	WARN_AND_RETURN_IF(!recreateSwapChain(), "Failed to create swapchain", false);

	data->barrierCommandList = data->device->_getNvDevice()->createCommandList();
	data->presentSemaphore	 = data->device->_getVkDevice().createSemaphore(vk::SemaphoreCreateInfo{}).value;
	return true;
}

//-------------------------

void VulkanFrameContext::destroySwapChain() {
	if (data->device && data->device->_getVkDevice()) {
		auto res = data->device->_getVkDevice().waitIdle();

		if (data->swapChain) {
			data->device->_getVkDevice().destroy(data->swapChain);
		}
	}
	data->swapChain = nullptr;
	data->swapChainImages.clear();
}

//-------------------------

bool VulkanFrameContext::recreateSwapChain() {
	destroySwapChain();

	auto physicalDevice = data->device->_getVkPhysicalDevice();
	auto vkDevice = data->device->_getVkDevice();
	auto nvDevice = data->device->_getNvDevice();
	auto capabilities = physicalDevice.getSurfaceCapabilitiesKHR(data->surface);
	auto formats			= physicalDevice.getSurfaceFormatsKHR(data->surface);
	auto presentModes = physicalDevice.getSurfacePresentModesKHR(data->surface);

	auto swapChainFormat							 = nvrhi::Format::SBGRA8_UNORM;
	vk::SurfaceFormatKHR surfaceFormat = {
		vk::Format(nvrhi::vulkan::convertFormat(swapChainFormat)),
		vk::ColorSpaceKHR::eSrgbNonlinear};

	const auto [backBufferWidth, backBufferHeight] = window->getFramebufferSize();
	vk::Extent2D extent														 = vk::Extent2D(backBufferWidth, backBufferHeight);

	std::unordered_set<uint32_t> uniqueQueues = {
		uint32_t(data->device->_getVkQueueFamilyIndex(QueueFamily::Graphics)),
		uint32_t(data->device->_getVkQueueFamilyIndex(QueueFamily::Present))};

	std::vector<uint32_t> queues(uniqueQueues.begin(), uniqueQueues.end());
	bool enableSwapChainSharing = queues.size() > 1;

	// TODO: make these configurable (in window?)
	bool vsyncEnabled							= true;
	uint32_t swapChainBufferCount = 2;

	auto desc = vk::SwapchainCreateInfoKHR()
		.setSurface(data->surface)
		.setMinImageCount(swapChainBufferCount)
		.setImageFormat(surfaceFormat.format)
		.setImageColorSpace(surfaceFormat.colorSpace)
		.setImageExtent(extent)
		.setImageArrayLayers(1)
		.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled)
		.setImageSharingMode(enableSwapChainSharing ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive)
		.setQueueFamilyIndexCount(enableSwapChainSharing ? static_cast<uint32_t>(queues.size()) : 0)
		.setPQueueFamilyIndices(enableSwapChainSharing ? queues.data() : nullptr)
		.setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity)
		.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
		.setPresentMode(vsyncEnabled ? vk::PresentModeKHR::eFifo : vk::PresentModeKHR::eImmediate)
		.setClipped(true)
		.setOldSwapchain(nullptr);

	vk::Result res;
	std::tie(res, data->swapChain) = vkDevice.createSwapchainKHR(desc);
	WARN_AND_RETURN_IF(res != vk::Result::eSuccess, StringUtils::format("Failed to create a Vulkan swap chain, error code = %s", nvrhi::vulkan::resultToString(res)), false);

	// retrieve swap chain images
	auto images = vkDevice.getSwapchainImagesKHR(data->swapChain).value;
	for (auto image : images) {
		VulkanSwapChainImage sci;
		sci.image = image;

		nvrhi::TextureDesc textureDesc;
		textureDesc.width						 = backBufferWidth;
		textureDesc.height					 = backBufferHeight;
		textureDesc.format					 = swapChainFormat;
		textureDesc.debugName				 = "Swap chain image";
		textureDesc.initialState		 = nvrhi::ResourceStates::Present;
		textureDesc.keepInitialState = true;
		textureDesc.isRenderTarget	 = true;

		sci.rhiHandle = nvDevice->createHandleForNativeTexture(nvrhi::ObjectTypes::VK_Image, nvrhi::Object(sci.image), textureDesc);
		data->swapChainImages.push_back(sci);
	}

	data->swapChainIndex = 0;
	return true;
}

//-------------------------

} // Rendering
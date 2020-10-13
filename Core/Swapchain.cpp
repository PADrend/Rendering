/*
	This file is part of the Rendering library.
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "Swapchain.h"
#include "Device.h"
#include "Queue.h"
#include "ImageStorage.h"
#include "Sampler.h"

#include "../FBO.h"
#include "../Texture/Texture.h"

#include <Util/UI/Window.h>
#include <Util/Macros.h>

#include <vulkan/vulkan.hpp>

#include <set>
namespace Rendering {

//------------

Swapchain::~Swapchain() {
	// Wait for fences before destroying swapchain
	vk::Device d(device->getApiHandle());
	for(auto& fence : presentFences) {
		vk::Fence f(fence);
		d.waitForFences(1, &f, false, std::numeric_limits<uint64_t>::max());
	}
}

//------------

Swapchain::Swapchain(const DeviceRef& device, const Geometry::Vec2ui& extent) : device(device), extent(extent) { }

//------------

bool Swapchain::init() {
	vk::Device vkDevice(device->getApiHandle());
	vk::PhysicalDevice physicalDevice(device->getApiHandle());
	vk::SurfaceKHR vkSurface(device->getSurface());
	
	// create swap chain
	auto capabilities = physicalDevice.getSurfaceCapabilitiesKHR(vkSurface);
	auto formats = physicalDevice.getSurfaceFormatsKHR(vkSurface);
	auto presentModes = physicalDevice.getSurfacePresentModesKHR(vkSurface);
	vk::SurfaceFormatKHR surfaceFormat;
	surfaceFormat.format = vk::Format::eB8G8R8A8Unorm;
	surfaceFormat.colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
	
	if(std::find(formats.begin(), formats.end(), surfaceFormat) == formats.end()) 
		return false;
		
  // Select present mode
	vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;
	for(auto& mode : presentModes) {
		if(mode == vk::PresentModeKHR::eMailbox) {
			presentMode = vk::PresentModeKHR::eMailbox;
			break;
		} else if(mode == vk::PresentModeKHR::eImmediate) {
			presentMode = vk::PresentModeKHR::eImmediate;
		}
	}
	
	// Set extent
	extent.setValue(
		std::max(capabilities.minImageExtent.width, std::min<uint32_t>(capabilities.maxImageExtent.width, extent.x())),
		std::max(capabilities.minImageExtent.height, std::min<uint32_t>(capabilities.maxImageExtent.height, extent.y()))
	);
	vk::Extent2D vkExtent(extent.x(), extent.y());
	
	// Set swapchain size
	imageCount = capabilities.minImageCount + 1;
	if(capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
		imageCount = capabilities.maxImageCount;
	
	vk::SwapchainCreateInfoKHR swapchainCreateInfo { {},
		vkSurface, imageCount, surfaceFormat.format, surfaceFormat.colorSpace,
		vkExtent, 1, vk::ImageUsageFlagBits::eColorAttachment,
		vk::SharingMode::eExclusive, 0, nullptr,
		capabilities.currentTransform, vk::CompositeAlphaFlagBitsKHR::eOpaque,
		presentMode, true
	};

	std::vector<uint32_t> familyIndices;
	auto queues = device->getQueues();
	for(auto& queue : device->getQueues()) {
		familyIndices.emplace_back(queue->getFamilyIndex());
	}
	if(familyIndices.size() > 1) {
		swapchainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
		swapchainCreateInfo.queueFamilyIndexCount = familyIndices.size();
		swapchainCreateInfo.pQueueFamilyIndices = familyIndices.data();
	}
	auto vkSwapchain = vkDevice.createSwapchainKHR(swapchainCreateInfo);
	if(!vkSwapchain)
		return false;
	handle = SwapchainHandle::create(vkSwapchain, vkDevice);
	
	auto swapchainImages = vkDevice.getSwapchainImagesKHR(vkSwapchain);
	imageCount = static_cast<uint32_t>(swapchainImages.size());

	// create present fences
	currentFence = 0;
	presentFences.clear();
	for(uint32_t i=0; i<imageCount; ++i) {
		presentFences.emplace_back(FenceHandle::create(vkDevice.createFence({vk::FenceCreateFlagBits::eSignaled}), vkDevice));
	}
	
	return updateFramebuffers();
}

//------------

bool Swapchain::updateFramebuffers() {
	vk::Device vkDevice(handle);
	vk::SwapchainKHR vkSwapchain(handle);
	auto swapchainImages = vkDevice.getSwapchainImagesKHR(vkSwapchain);
	ImageFormat format{};
	format.extent = {extent.x(), extent.y(), 1u};
	format.pixelFormat = InternalFormat::BGRA8Unorm;

	// Update FBOs
	fbos.resize(imageCount);
	for(uint32_t i=0; i<imageCount; ++i) {
		auto imageHandle = ImageHandle::create(swapchainImages[i], vkDevice);
		
		auto image = ImageStorage::createFromHandle(device.get(), {format, MemoryUsage::GpuOnly, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT}, std::move(imageHandle));
		auto texture = Texture::create(device.get(), image);
		auto& fbo = fbos[i];
		if(!fbo) fbo = FBO::create(device.get());
		fbo->attachColorTexture(texture);
		// TODO: create & add depth texture
		if(!fbo || !fbo->validate()) {
			WARN("Device: Could not create swap chain framebuffers.");
			return false;
		}
	}
	acquireNextIndex();
	return true;
}

//------------

uint32_t Swapchain::acquireNextIndex() {
	vk::Fence fence(presentFences[currentFence]);
	vk::Device vkDevice(handle);
	vk::SwapchainKHR vkSwapchain(handle);
	vkDevice.waitForFences(1, &fence, false, std::numeric_limits<uint64_t>::max());
	
	currentFence = (currentFence + 1) % imageCount;
	fence = presentFences[currentFence];
	vkDevice.resetFences(1, &fence);
	auto result = vkDevice.acquireNextImageKHR(vkSwapchain, std::numeric_limits<uint64_t>::max(), nullptr, fence);
	currentIndex = result.value;
	return currentIndex;
}

//------------

bool Swapchain::resize(uint32_t width, uint32_t height) {
	// TODO
	return false;
}

//------------
} /* Rendering */


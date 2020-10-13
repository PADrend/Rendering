/*
	This file is part of the Rendering library.
  Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "Device.h"
#include "Queue.h"
#include "Swapchain.h"
#include "CommandBuffer.h"
#include "CommandPool.h"

#include <Util/Macros.h>
#include <Util/UI/Window.h>
#include <Util/StringUtils.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <vulkan/vulkan.hpp>

#include <map>
#include <iostream>

namespace Rendering {

static Device* defaultDevice = nullptr;

//=========================================================================

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {

	std::cerr << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

//=========================================================================

struct Device::InternalData {
	explicit InternalData(Util::UI::WindowRef window) : window(std::move(window)) {}
	~InternalData() {
		if(debugMessenger) {
			vk::Instance ins(instance);
			ins.destroyDebugUtilsMessengerEXT(debugMessenger, nullptr, dldy);
		}
	}
	
	Util::UI::WindowRef window;
	InstanceHandle instance;
	DeviceHandle apiHandle;
	SurfaceHandle surface;
	AllocatorHandle allocator;
	Swapchain::Ref swapchain;

	vk::DispatchLoaderDynamic dldy;
	vk::PhysicalDevice physicalDevice;
	vk::DebugUtilsMessengerEXT debugMessenger = nullptr;	
	vk::PhysicalDeviceProperties properties;
	std::vector<std::string> extensions;

	std::map<Queue::Family, int32_t> familyIndices;
	std::vector<Queue::Ref> queues;
	std::vector<CommandPool::Ref> commandPools;
	
	bool createInstance(const Device::Ref& device, const Device::Configuration& config);
	bool initPhysicalDevice(const Device::Ref& device, const Device::Configuration& config);	
	bool createLogicalDevice(const Device::Ref& device, const Device::Configuration& config);
	bool createAllocator(const Device::Ref& device, const Device::Configuration& config);
	bool createMemoryAllocator(const Device::Ref& device, const Device::Configuration& config);
	bool createSwapchain(const Device::Ref& device, const Device::Configuration& config);
};

//=========================================================================

bool Device::InternalData::createInstance(const Device::Ref& device, const Device::Configuration& config) {
	if(config.debugMode)
		std::cout << "Creating Vulkan instance..." << std::endl;

	vk::ApplicationInfo appInfo(
		config.name.c_str(), 1,
		nullptr, 0,
		VK_API_VERSION_1_1
	);

	std::vector<const char*> layerNames;
	if(config.debugMode)
		layerNames.emplace_back("VK_LAYER_LUNARG_standard_validation");
	std::vector<const char*> requiredExtensions = window->getAPIExtensions();
	if(config.debugMode)
		requiredExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	vk::Instance vkInstance = vk::createInstance({{},
		&appInfo,
		static_cast<uint32_t>(layerNames.size()),
		layerNames.data(),
		static_cast<uint32_t>(requiredExtensions.size()),
		requiredExtensions.data()
	});
	if(!vkInstance)
		return false;
	
	instance = std::move(InstanceHandle(vkInstance));
	
	// setup debug callback
	dldy.init(vkInstance, vkGetInstanceProcAddr);
	if(config.debugMode) {		
		debugMessenger = vkInstance.createDebugUtilsMessengerEXT({ {},
			//vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning,
			vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
			vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
			vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
			debugCallback
		}, nullptr, dldy);
	}
	return true;
}

//------------

bool Device::InternalData::initPhysicalDevice(const Device::Ref& device, const Device::Configuration& config) {
	if(config.debugMode)
		std::cout << "Initializing physical device..." << std::endl;
	vk::Instance vkInstance(instance);

	// enumerate physical device
	auto physicalDevices = vkInstance.enumeratePhysicalDevices();
	if(physicalDevices.empty())
		return false;
	
	// Select best physical device based on memory
	vk::DeviceSize bestMemory = 0;
	for(auto& device : physicalDevices) {
		auto properties = device.getMemoryProperties();
		vk::DeviceSize deviceMemory = 0;
		for(uint32_t i=0; i<properties.memoryHeapCount; ++i) {
			if(properties.memoryHeaps[i].flags & vk::MemoryHeapFlagBits::eDeviceLocal) {
				deviceMemory = properties.memoryHeaps[i].size;
				break;
			}
		}
		
		if(!physicalDevice || deviceMemory > bestMemory) {
			bestMemory = deviceMemory;
			physicalDevice = device;
		}
	}
	if(!physicalDevice)
		return false;
	properties = physicalDevice.getProperties();
	
	// check API version
	uint32_t apiVersion = VK_MAKE_VERSION(config.apiVersionMajor, config.apiVersionMinor, 0);
	if(apiVersion > 0 && properties.apiVersion < apiVersion) {
		std::string reqVerStr = std::to_string(config.apiVersionMajor) + "." + std::to_string(config.apiVersionMinor);
		std::string supportedStr = std::to_string(VK_VERSION_MAJOR(properties.apiVersion)) + "." + std::to_string(VK_VERSION_MINOR(properties.apiVersion));
		WARN("Device: Requested API version is not supported. Requested version: " + reqVerStr + ", Highest supported: " + supportedStr);
		return false;
	}

	// get supported extensions
	auto extensionProperties = physicalDevice.enumerateDeviceExtensionProperties();
	for(auto& property : extensionProperties) {
		extensions.emplace_back(property.extensionName);
	}
	
	return true;
}

//------------

bool Device::InternalData::createLogicalDevice(const Device::Ref& device, const Device::Configuration& config) {
	if(config.debugMode)
		std::cout << "Creating logical device..." << std::endl;
	vk::SurfaceKHR vkSurface(surface);
	vk::Instance vkInstance(instance);

	familyIndices[Queue::Family::None] = -1;
	familyIndices[Queue::Family::Graphics] = -1;
	familyIndices[Queue::Family::Compute] = -1;
	familyIndices[Queue::Family::Transfer] = -1;
	familyIndices[Queue::Family::Present] = -1;
	
	// get queue families
	auto queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
	for(uint32_t i=0; i<queueFamilyProperties.size(); ++i) {
		if(familyIndices[Queue::Family::Graphics] < 0 && (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics)) {
			familyIndices[Queue::Family::Graphics] = i;
			if(physicalDevice.getSurfaceSupportKHR(i, vkSurface))
				familyIndices[Queue::Family::Present] = i;
		} else if(familyIndices[Queue::Family::Compute] < 0 && (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eCompute)) {
			familyIndices[Queue::Family::Compute] = i;
		} else if(familyIndices[Queue::Family::Transfer] < 0 && (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eTransfer)) {
			familyIndices[Queue::Family::Transfer] = i;
		}
	}

	// Create unique QueueCreateInfos
	float queuePriority = 1.0f;
	std::set<int32_t> uniqueIndices = {familyIndices[Queue::Family::Graphics], familyIndices[Queue::Family::Compute], familyIndices[Queue::Family::Transfer]};
	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
	for(int32_t index : uniqueIndices)
		queueCreateInfos.emplace_back(vk::DeviceQueueCreateFlags(), index, 1, &queuePriority);
	
	// set required extensions
	std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME }; // TODO: headless device?

	// check memory extensions
	if(device->isExtensionSupported("VK_KHR_get_memory_requirements2") && device->isExtensionSupported("VK_KHR_dedicated_allocation")) {
		deviceExtensions.emplace_back("VK_KHR_get_memory_requirements2");
		deviceExtensions.emplace_back("VK_KHR_dedicated_allocation");
	}

	// create logical device
	auto vkDevice = physicalDevice.createDevice({ {}, 
		static_cast<uint32_t>(queueCreateInfos.size()), queueCreateInfos.data(), 
		0, nullptr, 
		static_cast<uint32_t>(deviceExtensions.size()), deviceExtensions.data()
	}, nullptr, dldy);

	if(!vkDevice)
		return false;
	
	// Create handle
	dldy.init(vkInstance, vkDevice);
	apiHandle = std::move(DeviceHandle(vkDevice, physicalDevice));

	// Create command queues & pools
	queues.clear();
	queues.resize(queueFamilyProperties.size());
	commandPools.clear();
	commandPools.resize(queueFamilyProperties.size());
	for(int32_t index : uniqueIndices) {
		queues[index] = new Queue(device, index, 0); // For now, we only support one queue per family.
		commandPools[index] = new CommandPool(device, index);
	}

	return true;
}

//------------

bool Device::InternalData::createMemoryAllocator(const Device::Ref& device, const Device::Configuration& config){
	if(config.debugMode)
		std::cout << "Creating memory allocator..." << std::endl;
	VmaVulkanFunctions vmaVulkanFunc{};
	vmaVulkanFunc.vkAllocateMemory                    = dldy.vkAllocateMemory;
	vmaVulkanFunc.vkBindBufferMemory                  = dldy.vkBindBufferMemory;
	vmaVulkanFunc.vkBindImageMemory                   = dldy.vkBindImageMemory;
	vmaVulkanFunc.vkCreateBuffer                      = dldy.vkCreateBuffer;
	vmaVulkanFunc.vkCreateImage                       = dldy.vkCreateImage;
	vmaVulkanFunc.vkDestroyBuffer                     = dldy.vkDestroyBuffer;
	vmaVulkanFunc.vkDestroyImage                      = dldy.vkDestroyImage;
	vmaVulkanFunc.vkFlushMappedMemoryRanges           = dldy.vkFlushMappedMemoryRanges;
	vmaVulkanFunc.vkFreeMemory                        = dldy.vkFreeMemory;
	vmaVulkanFunc.vkGetBufferMemoryRequirements       = dldy.vkGetBufferMemoryRequirements;
	vmaVulkanFunc.vkGetImageMemoryRequirements        = dldy.vkGetImageMemoryRequirements;
	vmaVulkanFunc.vkGetPhysicalDeviceMemoryProperties	= dldy.vkGetPhysicalDeviceMemoryProperties;
	vmaVulkanFunc.vkGetPhysicalDeviceProperties       = dldy.vkGetPhysicalDeviceProperties;
	vmaVulkanFunc.vkInvalidateMappedMemoryRanges      = dldy.vkInvalidateMappedMemoryRanges;
	vmaVulkanFunc.vkMapMemory                         = dldy.vkMapMemory;
	vmaVulkanFunc.vkUnmapMemory                       = dldy.vkUnmapMemory;

	VmaAllocatorCreateInfo allocatorInfo{};
	allocatorInfo.physicalDevice = device->getApiHandle();
	allocatorInfo.device = device->getApiHandle();

	if(device->isExtensionSupported("VK_KHR_get_memory_requirements2") && device->isExtensionSupported("VK_KHR_dedicated_allocation")) {
		allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
		vmaVulkanFunc.vkGetBufferMemoryRequirements2KHR = dldy.vkGetBufferMemoryRequirements2KHR;
		vmaVulkanFunc.vkGetImageMemoryRequirements2KHR  = dldy.vkGetImageMemoryRequirements2KHR;
	}

	allocatorInfo.pVulkanFunctions = &vmaVulkanFunc;

	VmaAllocator vmaAllocator = nullptr;
	if(vmaCreateAllocator(&allocatorInfo, &vmaAllocator) != VK_SUCCESS)
		return false;

	allocator = std::move(AllocatorHandle(vmaAllocator, apiHandle));
	return true;
}

//------------

bool Device::InternalData::createSwapchain(const Device::Ref& device, const Device::Configuration& config) {
	if(config.debugMode)
		std::cout << "Creating swapchain..." << std::endl;
	swapchain = new Swapchain(device, {static_cast<int32_t>(window->getWidth()), static_cast<int32_t>(window->getHeight())});
	if(!swapchain->init()) {
		swapchain = nullptr;
		return false;
	}
	return true;
}

//=========================================================================

Device::Ref Device::create(Util::UI::WindowRef window, const Configuration& config) {
	Ref device = new Device(std::move(window));
	if(!device->init(config))
		device = nullptr;
	if(!defaultDevice)
		defaultDevice = device.get();
	return device;
}

//------------

Device::Ref Device::getDefault() {
	return defaultDevice;
}

//------------

Device::Device(Util::UI::WindowRef window) : internal(new InternalData(std::move(window))) { }

//------------

Device::~Device() = default;

//------------

const SwapchainRef& Device::getSwapchain() const {
	return internal->swapchain;
}

//------------

void Device::present() {
	internal->queues[internal->familyIndices[Queue::Family::Present]]->present();
}

//------------

void Device::flush() {
	
}

//------------

bool Device::isExtensionSupported(const std::string& extension) const {
	return std::find(internal->extensions.begin(), internal->extensions.end(), extension) != internal->extensions.end();
}

//------------

uint32_t Device::getMaxFramebufferAttachments() const {
	return internal->properties.limits.maxColorAttachments;
}

//------------

const Util::UI::WindowRef& Device::getWindow() const {
	return internal->window;
}

//------------

const AllocatorHandle& Device::getAllocator() const {
	return internal->allocator;
}

//------------

const Queue::Ref& Device::getQueue(Queue::Family family, uint32_t index) const {
	static Queue::Ref nullRef;
	if(index > 0)
		WARN("Device:getQueue: Only one queue per family is supported.");
	auto familyIndex = internal->familyIndices[family];
	if(familyIndex < 0) {
		WARN("Device:getQueue: Unsupported family index " + Util::StringUtils::toString(familyIndex) + ".");
		return nullRef;
	}
	return familyIndex >= 0 ? internal->queues[familyIndex] : nullRef;
}


//------------

const Queue::Ref& Device::getQueue(uint32_t familyIndex, uint32_t index) const {
	static Queue::Ref nullRef;
	if(familyIndex >= internal->queues.size())
		return nullRef;
	if(index > 0)
		WARN("Device:getQueue: Only one queue per family is supported.");
	return internal->queues[familyIndex];
}

//------------

std::set<Queue::Ref> Device::getQueues() const {
	std::set<Queue::Ref> queues;
	for(auto& queue : internal->queues) {
		if(queue)
			queues.emplace(queue);
	}
	return queues;
}

//------------

const CommandPoolRef& Device::getCommandPool(Queue::Family family) const {
	static CommandPoolRef nullRef;
	auto familyIndex = internal->familyIndices[family];
	return familyIndex >= 0 ? internal->commandPools[familyIndex] : nullRef;
}

//------------

const CommandPoolRef& Device::getCommandPool(uint32_t familyIndex) const {
	static CommandPoolRef nullRef;
	if(familyIndex >= internal->commandPools.size())
		return nullRef;
	return internal->commandPools[familyIndex];
}

//------------

const SurfaceHandle& Device::getSurface() const {
	return internal->surface;
}

//------------

const InstanceHandle& Device::getInstance() const {
	return internal->instance;
}

//------------

const DeviceHandle& Device::getApiHandle() const {
	return internal->apiHandle;
}

//------------

bool Device::init(const Configuration& config) {
	
	if(!internal->createInstance(this, config)) {
		WARN("Device: Could not create Vulkan instance.");
		return false;
	}

	if(!internal->initPhysicalDevice(this, config)) {
		WARN("Device: Could not create Vulkan physical device.");
		return false;
	}

	if(config.debugMode)
		std::cout << "Aquirering window surface..." << std::endl;
	vk::SurfaceKHR surface(internal->window->createSurface(internal->instance));
	if(!surface) {
		WARN("Device: Could not create Vulkan surface.");
		return false;
	}
	internal->surface = std::move(SurfaceHandle(surface, internal->instance));

	if(!internal->createLogicalDevice(this, config)) {
		WARN("Device: Could not create Vulkan device.");
		return false;
	}	
	
	if(!internal->createMemoryAllocator(this, config)) {
		WARN("Device: Could not create memory allocator.");
		return false;
	}
		
	if(!internal->createSwapchain(this, config)) {
		WARN("Device: Could not create Swapchain.");
		return false;
	}
	
	return true;
}

} /* Rendering */
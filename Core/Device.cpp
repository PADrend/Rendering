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
#include "ResourceCache.h"
#include "DescriptorPool.h"

#include <Util/Macros.h>
#include <Util/UI/Window.h>
#include <Util/StringUtils.h>
#include <Util/Factory/ObjectCache.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#include <map>
#include <iostream>

namespace Rendering {

static Device* defaultDevice = nullptr;

static uint64_t getDeviceScore(const vk::PhysicalDevice& device) {
	uint64_t score = 1;
	auto props = device.getProperties();
	auto memProps = device.getMemoryProperties();
	
	switch(props.deviceType) {
		case vk::PhysicalDeviceType::eDiscreteGpu: score <<= 63; break; // prefer discrete gpu
		case vk::PhysicalDeviceType::eIntegratedGpu: score <<= 62; break;
		default: score <<= 61;
	}

	uint64_t deviceMemory = 0;
	for(uint32_t i=0; i<memProps.memoryHeapCount; ++i) {
		if(memProps.memoryHeaps[i].flags & vk::MemoryHeapFlagBits::eDeviceLocal) {
			deviceMemory = memProps.memoryHeaps[i].size;
			break;
		}
	}
	return score | (deviceMemory >> 3);
}

//=========================================================================

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData
) {
	std::ostringstream message;
	message << vk::to_string(static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(messageSeverity)) << ":";
	message << " <" << pCallbackData->pMessageIdName << ">";
	message << " " << vk::to_string(static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(messageTypes)) << std::endl;
	message << "  Message: \"" << pCallbackData->pMessage << "\"" << std::endl;
	if(pCallbackData->queueLabelCount > 0) {
		message << "  " << "Queue Labels:" << std::endl;
		for(uint_fast32_t i = 0; i < pCallbackData->queueLabelCount; ++i) {
			message << "    " << pCallbackData->pQueueLabels[i].pLabelName << std::endl;
		}
	}
	if(pCallbackData->cmdBufLabelCount > 0) {
		message << "  " << "CommandBuffer Labels:" << std::endl;
		for(uint_fast32_t i = 0; i < pCallbackData->cmdBufLabelCount; ++i) {
			message << "    " << pCallbackData->pCmdBufLabels[i].pLabelName << std::endl;
		}
	}
	if(pCallbackData->objectCount > 0) {
		message << "  " << "Objects:" << std::endl;
		for(uint_fast32_t i = 0; i < pCallbackData->objectCount; ++i) {
			message << "    " << vk::to_string(static_cast<vk::ObjectType>(pCallbackData->pObjects[i].objectType));
			message << "(" << pCallbackData->pObjects[i].objectHandle << ")";
			if(pCallbackData->pObjects[i].pObjectName) {
				message << " \"" << pCallbackData->pObjects[i].pObjectName << "\"";
			}
			message << std::endl;
		}
	}
	std::cout << message.str() << std::endl;
	return VK_FALSE;
}

//=========================================================================

struct Device::InternalData {
	explicit InternalData(Util::UI::WindowRef window) : window(std::move(window)) {}
	~InternalData() {
		if(debugMessenger) {
			vk::Instance ins(instance);
			ins.destroyDebugUtilsMessengerEXT(debugMessenger);
		}
	}
	
	Util::UI::WindowRef window;
	InstanceHandle instance;
	DeviceHandle apiHandle;
	SurfaceHandle surface;
	AllocatorHandle allocator;
	Swapchain::Ref swapchain;
	ResourceCache::Ref resourceCache;
	PipelineCacheHandle pipelineCache;
	DescriptorPool::Ref descriptorPool;

	vk::PhysicalDevice physicalDevice;
	vk::DebugUtilsMessengerEXT debugMessenger = nullptr;	
	vk::PhysicalDeviceProperties properties;
	std::vector<std::string> extensions;

	std::map<QueueFamily, int32_t> familyIndices;
	std::vector<Queue::Ref> queues;
	
	bool createInstance(const Device::Ref& device, const Device::Configuration& config);
	bool initPhysicalDevice(const Device::Ref& device, const Device::Configuration& config);
	bool createLogicalDevice(const Device::Ref& device, const Device::Configuration& config);
	bool createMemoryAllocator(const Device::Ref& device, const Device::Configuration& config);
	bool createSwapchain(const Device::Ref& device, const Device::Configuration& config);
	bool createDescriptorPools(const Device::Ref& device, const Device::Configuration& config);
};

//=========================================================================

bool Device::InternalData::createInstance(const Device::Ref& device, const Device::Configuration& config) {
	if(config.debugMode)
		std::cout << "Creating Vulkan instance..." << std::endl;

	vk::DynamicLoader dl;
	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
	VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

	vk::ApplicationInfo appInfo(
		config.name.c_str(), 1,
		nullptr, 0,
		VK_API_VERSION_1_1
	);

	std::vector<const char*> layerNames;
	std::vector<const char*> requiredExtensions = window->getAPIExtensions();
	if(config.debugMode) {
		layerNames.emplace_back("VK_LAYER_LUNARG_standard_validation");
		for(auto& layer : config.validationLayers)
			layerNames.emplace_back(layer.c_str());
		requiredExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		// print available and selected layers
		std::cout << "Validation layers:" << std::endl;
		auto availableLayers = vk::enumerateInstanceLayerProperties();
		for(auto& p : availableLayers) {
			bool found = false;
			for(auto& layer : layerNames) {
				if(std::strcmp(layer, p.layerName) == 0) {
					found = true;
					break;
				}
			}
			std::cout << "  " << p.layerName << (found ? " - enabled" : " - disabled") << std::endl;
		}
		for(auto& layer : layerNames) {
			bool found = false;
			for(auto& p : availableLayers) {
				if(std::strcmp(layer, p.layerName) == 0) {
					found = true;
					break;
				}
			}
			if(!found)
				std::cout << "  " << layer << " - not found" << std::endl;
		}
		
		// print available and selected extensions
		std::cout << "Extensions:" << std::endl;
		auto availableExt = vk::enumerateInstanceExtensionProperties();
		for(auto& p : availableExt) {
			bool found = false;
			for(auto& ext : requiredExtensions) {
				if(std::strcmp(ext, p.extensionName) == 0) {
					found = true;
					break;
				}
			}
			std::cout << "  " << p.extensionName << (found ? " - enabled" : " - disabled") << std::endl;
		}
		for(auto& ext : requiredExtensions) {
			bool found = false;
			for(auto& p : availableExt) {
				if(std::strcmp(ext, p.extensionName) == 0) {
					found = true;
					break;
				}
			}
			if(!found)
				std::cout << "  " << ext << " - not found" << std::endl;
		}
	}

	vk::Instance vkInstance = vk::createInstance({{},
		&appInfo,
		static_cast<uint32_t>(layerNames.size()),
		layerNames.data(),
		static_cast<uint32_t>(requiredExtensions.size()),
		requiredExtensions.data()
	});
	if(!vkInstance)
		return false;
	
	instance = InstanceHandle::create(vkInstance);
	VULKAN_HPP_DEFAULT_DISPATCHER.init(vkInstance);
	
	
	// setup debug callback
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
		});
		
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
	std::sort(physicalDevices.begin(), physicalDevices.end(), [](const vk::PhysicalDevice& d1, const vk::PhysicalDevice& d2) {
		return getDeviceScore(d1) > getDeviceScore(d2);
	});
	physicalDevice = physicalDevices.front();
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

	if(config.debugMode) {
		std::cout << "Vulkan version: "
							<< std::to_string(VK_VERSION_MAJOR(properties.apiVersion)) << "."
							<< std::to_string(VK_VERSION_MINOR(properties.apiVersion)) << "."
							<< std::to_string(VK_VERSION_PATCH(properties.apiVersion)) << std::endl;
		std::cout << "Selected device: " << properties.deviceName << std::endl;
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

	familyIndices[QueueFamily::None] = -1;
	familyIndices[QueueFamily::Graphics] = -1;
	familyIndices[QueueFamily::Compute] = -1;
	familyIndices[QueueFamily::Transfer] = -1;
	familyIndices[QueueFamily::Present] = -1;
	
	// get queue families
	auto queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
	for(uint32_t i=0; i<queueFamilyProperties.size(); ++i) {
		if(familyIndices[QueueFamily::Graphics] < 0 && (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics)) {
			familyIndices[QueueFamily::Graphics] = i;
		}
		if(familyIndices[QueueFamily::Compute] < 0 && (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eCompute)) {
			familyIndices[QueueFamily::Compute] = i;
		}
		if(familyIndices[QueueFamily::Transfer] < 0 && (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eTransfer)) {
			familyIndices[QueueFamily::Transfer] = i;
		}
		if(familyIndices[QueueFamily::Present] < 0 && physicalDevice.getSurfaceSupportKHR(i, vkSurface)) {
			familyIndices[QueueFamily::Present] = i;
		}
	}

	// Create unique QueueCreateInfos
	float queuePriority = 1.0f;
	std::set<int32_t> uniqueIndices = {familyIndices[QueueFamily::Graphics], familyIndices[QueueFamily::Compute], familyIndices[QueueFamily::Transfer]};
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
	});

	if(!vkDevice)
		return false;
	
	// Create handle
	VULKAN_HPP_DEFAULT_DISPATCHER.init(vkInstance, vkDevice);
	apiHandle = DeviceHandle::create(vkDevice, physicalDevice);

	// Create command queues & pools
	queues.clear();
	queues.resize(queueFamilyProperties.size());
	for(int32_t index : uniqueIndices) {
		auto queue = new Queue(device, index, 0); // For now, we only support one queue per family.
		if(!queue->init()) {
			WARN("Device: Could not create command queue.");
			return false;
		}
		queues[index] = queue;
	}

	// Create pipeline cache
	pipelineCache = PipelineCacheHandle::create(vkDevice.createPipelineCache({}), vkDevice);

	// Create resource cache
	resourceCache = ResourceCache::create(device);

	return true;
}

//------------

bool Device::InternalData::createMemoryAllocator(const Device::Ref& device, const Device::Configuration& config){
	if(config.debugMode)
		std::cout << "Creating memory allocator..." << std::endl;
	VmaVulkanFunctions vmaVulkanFunc{};
	vmaVulkanFunc.vkAllocateMemory                    = VULKAN_HPP_DEFAULT_DISPATCHER.vkAllocateMemory;
	vmaVulkanFunc.vkBindBufferMemory                  = VULKAN_HPP_DEFAULT_DISPATCHER.vkBindBufferMemory;
	vmaVulkanFunc.vkBindImageMemory                   = VULKAN_HPP_DEFAULT_DISPATCHER.vkBindImageMemory;
	vmaVulkanFunc.vkCreateBuffer                      = VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateBuffer;
	vmaVulkanFunc.vkCreateImage                       = VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateImage;
	vmaVulkanFunc.vkDestroyBuffer                     = VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyBuffer;
	vmaVulkanFunc.vkDestroyImage                      = VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyImage;
	vmaVulkanFunc.vkFlushMappedMemoryRanges           = VULKAN_HPP_DEFAULT_DISPATCHER.vkFlushMappedMemoryRanges;
	vmaVulkanFunc.vkFreeMemory                        = VULKAN_HPP_DEFAULT_DISPATCHER.vkFreeMemory;
	vmaVulkanFunc.vkGetBufferMemoryRequirements       = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetBufferMemoryRequirements;
	vmaVulkanFunc.vkGetImageMemoryRequirements        = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetImageMemoryRequirements;
	vmaVulkanFunc.vkGetPhysicalDeviceMemoryProperties	= VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceMemoryProperties;
	vmaVulkanFunc.vkGetPhysicalDeviceProperties       = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceProperties;
	vmaVulkanFunc.vkInvalidateMappedMemoryRanges      = VULKAN_HPP_DEFAULT_DISPATCHER.vkInvalidateMappedMemoryRanges;
	vmaVulkanFunc.vkMapMemory                         = VULKAN_HPP_DEFAULT_DISPATCHER.vkMapMemory;
	vmaVulkanFunc.vkUnmapMemory                       = VULKAN_HPP_DEFAULT_DISPATCHER.vkUnmapMemory;

	VmaAllocatorCreateInfo allocatorInfo{};
	allocatorInfo.physicalDevice = device->getApiHandle();
	allocatorInfo.device = device->getApiHandle();

	if(device->isExtensionSupported("VK_KHR_get_memory_requirements2") && device->isExtensionSupported("VK_KHR_dedicated_allocation")) {
		allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
		vmaVulkanFunc.vkGetBufferMemoryRequirements2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetBufferMemoryRequirements2KHR;
		vmaVulkanFunc.vkGetImageMemoryRequirements2KHR  = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetImageMemoryRequirements2KHR;
	}

	allocatorInfo.pVulkanFunctions = &vmaVulkanFunc;

	VmaAllocator vmaAllocator = nullptr;
	if(vmaCreateAllocator(&allocatorInfo, &vmaAllocator) != VK_SUCCESS)
		return false;

	allocator = AllocatorHandle::create(vmaAllocator, apiHandle);
	return allocator.isNotNull();
}

//------------

bool Device::InternalData::createSwapchain(const Device::Ref& device, const Device::Configuration& config) {
	if(config.debugMode)
		std::cout << "Creating swapchain..." << std::endl;
	swapchain = new Swapchain(device, {window->getWidth(), window->getHeight()});
	if(!swapchain->init()) {
		swapchain = nullptr;
		return false;
	}
	return true;
}

//------------

bool Device::InternalData::createDescriptorPools(const Device::Ref& device, const Device::Configuration& config) {
	if(config.debugMode)
		std::cout << "Creating descriptor pools..." << std::endl;
	
	// Descriptor counts inspired by Falcor
	DescriptorPool::Configuration poolConfig;
	poolConfig.setDescriptorCount(ShaderResourceType::BufferStorage, 2 * 1024)
						.setDescriptorCount(ShaderResourceType::BufferUniform, 16 * 1024)
						.setDescriptorCount(ShaderResourceType::Image, 1000000)
						.setDescriptorCount(ShaderResourceType::ImageSampler, 1000000)
						.setDescriptorCount(ShaderResourceType::ImageStorage, 16 * 1024)
						.setDescriptorCount(ShaderResourceType::Sampler, 2 * 1024);
	
	descriptorPool = DescriptorPool::create(device, poolConfig);
	return descriptorPool.isNotNull();
}

//=========================================================================

Device::Ref Device::create(Util::UI::WindowRef window, const Configuration& config) {
	Ref device = new Device(std::move(window), config);
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

Device::Device(Util::UI::WindowRef window, const Configuration& config) : internal(new InternalData(std::move(window))), config(config) { }

//------------

Device::~Device() = default;

//------------

const SwapchainRef& Device::getSwapchain() const {
	return internal->swapchain;
}

//------------

void Device::present() {
	internal->queues[internal->familyIndices[QueueFamily::Present]]->present();
}

//------------

void Device::waitIdle() {
	vk::Device device(internal->apiHandle);
	device.waitIdle();
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

uint32_t Device::getMaxPushConstantSize() const {
	return internal->properties.limits.maxPushConstantsSize;
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

const Queue::Ref& Device::getQueue(QueueFamily family, uint32_t index) const {
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

const PipelineCacheHandle& Device::getPipelineCache() const {
	return internal->pipelineCache;
}

//------------

const ResourceCacheRef& Device::getResourceCache() const {
	return internal->resourceCache;
}

//------------

const SurfaceHandle& Device::getSurface() const {
	return internal->surface;
}

//------------

const DescriptorPoolRef& Device::getDescriptorPool() const {
	return internal->descriptorPool;
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
		std::cout << "Acquiring window surface..." << std::endl;
	vk::SurfaceKHR surface(internal->window->createSurface(internal->instance));
	if(!surface) {
		WARN("Device: Could not create Vulkan surface.");
		return false;
	}
	internal->surface = SurfaceHandle::create(surface, internal->instance);

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
		
	if(!internal->createDescriptorPools(this, config)) {
		WARN("Device: Could not create descriptor pools.");
		return false;
	}
	
	return true;
}

} /* Rendering */
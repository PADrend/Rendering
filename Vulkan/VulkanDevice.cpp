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
#include "VulkanDevice.h"
#include "VulkanFrameContext.h"

#include <Util/Hashing.h>
#include <Util/Macros.h>
#include <Util/StringUtils.h>
#include <Util/UI/UI.h>

#define VULKAN_HPP_NO_EXCEPTIONS
#include <nvrhi/nvrhi.h>
#include <nvrhi/validation.h>
#include <nvrhi/vulkan.h>

#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Rendering {
using namespace Util;
namespace {

//-------------------------

struct VulkanDeviceExtensionFeature {
	template<typename Feature>
	VulkanDeviceExtensionFeature(const std::string& extension, const Feature& feature) :
			extension(extension), featureData(sizeof(Feature)) {
		std::copy(reinterpret_cast<const uint8_t*>(&feature), reinterpret_cast<const uint8_t*>(&feature) + sizeof(Feature), featureData.data());
		next = &reinterpret_cast<Feature*>(featureData.data())->pNext;
	}
	std::string extension;
	void** next;
	std::vector<uint8_t> featureData;
};

//-------------------------

} // namespace

//-------------------------

struct VulkanDevice::InternalBufferResource {
};

//-------------------------

struct VulkanDevice::InternalImageResource {
};

//-------------------------

struct VulkanDevice::Internal {
	std::vector<const char*> extensions;
	std::unordered_map<Util::StringIdentifier, VulkanDeviceExtensionFeature> extensionFeatures;
	std::unordered_map<QueueFamily, int32_t> queueFamilies;
	std::unordered_map<QueueFamily, vk::Queue> queues;
	vk::PhysicalDevice physicalDevice = nullptr;
	vk::Device vkDevice								= nullptr;
	nvrhi::vulkan::DeviceHandle nvDevice;
	bool enabledSurfaceSupport;
	
	template<typename Feature>
	void registerExtensionFeature(const std::string& ext, const Feature& feature) {
		extensionFeatures.emplace(Util::StringIdentifier(ext), VulkanDeviceExtensionFeature(ext, feature));
	}
	void registerExtensionFeatures();
};

//-------------------------

void VulkanDevice::Internal::registerExtensionFeatures() {
	registerExtensionFeature(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME, vk::PhysicalDeviceSynchronization2FeaturesKHR()
		.setSynchronization2(true));
	registerExtensionFeature(VK_NV_MESH_SHADER_EXTENSION_NAME, vk::PhysicalDeviceMeshShaderFeaturesNV()
		.setMeshShader(true)
		.setTaskShader(true));
	registerExtensionFeature(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, vk::PhysicalDeviceBufferAddressFeaturesEXT()
		.setBufferDeviceAddress(true));
	registerExtensionFeature(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, vk::PhysicalDeviceAccelerationStructureFeaturesKHR()
		.setAccelerationStructure(true));
	registerExtensionFeature(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, vk::PhysicalDeviceRayTracingPipelineFeaturesKHR()
		.setRayTracingPipeline(true)
		.setRayTraversalPrimitiveCulling(true));
	registerExtensionFeature(VK_KHR_RAY_QUERY_EXTENSION_NAME, vk::PhysicalDeviceRayQueryFeaturesKHR()
		.setRayQuery(true));
	registerExtensionFeature(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME, vk::PhysicalDeviceFragmentShadingRateFeaturesKHR()
		.setPipelineFragmentShadingRate(true)
		.setPrimitiveFragmentShadingRate(true)
		.setAttachmentFragmentShadingRate(true));
	registerExtensionFeature(VK_EXT_CONDITIONAL_RENDERING_EXTENSION_NAME, vk::PhysicalDeviceConditionalRenderingFeaturesEXT()
		.setConditionalRendering(true));
}

//=======================================================================================================
// VulkanDevice

VulkanDevice::VulkanDevice(const VulkanDeviceConfig& config) :
		RenderDevice(), config(config), data(std::make_unique<Internal>()) {
}

//-------------------------

VulkanDevice::~VulkanDevice() {
	shutdown(); // might be dangerous because of virtual function
}

//-------------------------

VulkanDeviceHandle VulkanDevice::create(const VulkanDeviceConfig& config) {
	WARN_AND_RETURN_IF(!VulkanInstance::isValid(), "Invalid instance.", nullptr);
	auto instance				= VulkanInstance::_getVkInstance();
	auto physicalDevice = VulkanInstance::_getVkPhysicalDevice(config.physicalDeviceId);
	const auto& instanceConfig	= VulkanInstance::getConfig();
	WARN_AND_RETURN_IF(!physicalDevice, Util::StringUtils::format("Failed to find physical device '%s'", config.physicalDeviceId.toString()), nullptr);
	std::cout << "Creating Vulkan device..." << std::endl;
	std::cout << "  Physical device: " << config.physicalDeviceId.toString() << std::endl;

	VulkanDeviceHandle device		 = new VulkanDevice(config);
	device->data->physicalDevice = physicalDevice;

	//-----------------------------------------------------------
	// check available extensions

	device->data->extensions = {
		VK_KHR_MAINTENANCE1_EXTENSION_NAME
	};
	std::transform(config.extensions.cbegin(), config.extensions.cend(), std::back_inserter(device->data->extensions), [](const std::string& ext) {
		return ext.c_str();
	});

	auto availableExt = physicalDevice.enumerateDeviceExtensionProperties();

	std::cout << "  Device extensions:" << std::endl;
	bool allExtensionsFound = true;
	for (auto& ext : device->data->extensions) {
		bool found = false;
		for (auto& p : availableExt.value) {
			if (std::strcmp(ext, p.extensionName) == 0) {
				found = true;
				break;
			}
		}
		if (found && std::strcmp(ext, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
			device->data->enabledSurfaceSupport = true;
		}
		allExtensionsFound &= found;
		std::cout << "    " << ext << " - " << (found ? "found" : "not found") << std::endl;
	}
	WARN_AND_RETURN_IF(!allExtensionsFound, "Failed to create device. Missing extensions", nullptr);

	//-----------------------------------------------------------
	// find queue families

	device->data->queueFamilies[QueueFamily::None]		 = -1;
	device->data->queueFamilies[QueueFamily::Graphics] = -1;
	device->data->queueFamilies[QueueFamily::Compute]	 = -1;
	device->data->queueFamilies[QueueFamily::Transfer] = -1;
	device->data->queueFamilies[QueueFamily::Present]	 = -1;

	// get queue families
	QueueFamily usedFamilies = QueueFamily::None;
	auto queueFamily				 = physicalDevice.getQueueFamilyProperties();
	for (int32_t i = 0; i < static_cast<int32_t>(queueFamily.size()); ++i) {
		if (queueFamily[i].queueCount > 0) {
			if (device->data->queueFamilies[QueueFamily::Graphics] < 0 && (queueFamily[i].queueFlags & vk::QueueFlagBits::eGraphics)) {
				device->data->queueFamilies[QueueFamily::Graphics] = i;
				usedFamilies																			 = usedFamilies | QueueFamily::Graphics;
			}
			if (device->data->queueFamilies[QueueFamily::Compute] < 0 && (queueFamily[i].queueFlags & vk::QueueFlagBits::eCompute)) {
				device->data->queueFamilies[QueueFamily::Compute] = i;
				usedFamilies																			= usedFamilies | QueueFamily::Compute;
			}
			if (device->data->queueFamilies[QueueFamily::Transfer] < 0 && (queueFamily[i].queueFlags & vk::QueueFlagBits::eTransfer)) {
				device->data->queueFamilies[QueueFamily::Transfer] = i;
				usedFamilies																			 = usedFamilies | QueueFamily::Transfer;
			}
			if (device->data->enabledSurfaceSupport && device->data->queueFamilies[QueueFamily::Present] < 0 && Util::UI::getPhysicalDevicePresentationSupport(instance, physicalDevice, i)) {
				device->data->queueFamilies[QueueFamily::Present] = i;
				usedFamilies																			= usedFamilies | QueueFamily::Present;
			}
		}
	}

	std::cout << "  Queue families: " << toString(usedFamilies) << std::endl;

	// Create unique QueueCreateInfos
	float queuePriority												= 1.0f;
	std::unordered_set<int32_t> uniqueIndices = {
		device->data->queueFamilies[QueueFamily::Graphics],
		device->data->queueFamilies[QueueFamily::Compute],
		device->data->queueFamilies[QueueFamily::Transfer]};
	std::vector<vk::DeviceQueueCreateInfo> queueInfos;
	for (int32_t index : uniqueIndices)
		queueInfos.emplace_back(vk::DeviceQueueCreateFlags(), index, 1, &queuePriority);

	//-----------------------------------------------------------
	// init features

	device->data->registerExtensionFeatures();

	void* pNext = nullptr;
	for(const auto& ext : device->data->extensions) {
		if(auto it = device->data->extensionFeatures.find(Util::StringIdentifier(ext)); it != device->data->extensionFeatures.end()) {
			if(!it->second.featureData.empty()) {
				*it->second.next = pNext;
				pNext = it->second.featureData.data();
			}
		}
	}

	auto vk11Features = vk::PhysicalDeviceVulkan11Features()
												.setPNext(pNext);

	auto vk12Features = vk::PhysicalDeviceVulkan12Features()
												.setPNext(&vk11Features);

	auto pFeatures2 = vk::PhysicalDeviceFeatures2()
											.setPNext(&vk12Features);
	pFeatures2.pNext = &vk12Features;
	physicalDevice.getFeatures2(&pFeatures2);

	// some required features
	// TODO: error when not supported
	vk12Features
		.setHostQueryReset(true)
		.setDrawIndirectCount(true)
		.setUniformAndStorageBuffer8BitAccess(true)
		.setDescriptorIndexing(true)
		.setRuntimeDescriptorArray(true)
		.setDescriptorBindingPartiallyBound(true)
		.setDescriptorBindingVariableDescriptorCount(true)
		.setTimelineSemaphore(true)
		.setShaderSampledImageArrayNonUniformIndexing(true);

	auto deviceFeatures = vk::PhysicalDeviceFeatures()
		.setShaderImageGatherExtended(true)
		.setSamplerAnisotropy(true)
		.setTessellationShader(true)
		.setTextureCompressionBC(true)
		.setGeometryShader(true)
		.setImageCubeArray(true)
		.setDualSrcBlend(true);

	//-----------------------------------------------------------
	// create device

	auto deviceInfo = vk::DeviceCreateInfo()
		.setQueueCreateInfos(queueInfos)
		.setPEnabledFeatures(&deviceFeatures)
		.setPEnabledExtensionNames(device->data->extensions)
		.setPEnabledLayerNames(VulkanInstance::_getVkValidationLayers())
		.setPNext(&vk12Features);

	auto [result, vkDevice] = physicalDevice.createDevice(deviceInfo);
	WARN_AND_RETURN_IF(result != vk::Result::eSuccess, "Failed to create Vulkan device: " + vk::to_string(result), nullptr);
	device->data->vkDevice = vkDevice;

	for (const auto& family : device->data->queueFamilies) {
		if (family.second >= 0) {
			device->data->queues[family.first] = vkDevice.getQueue(family.second, 0);
		}
	}

	VULKAN_HPP_DEFAULT_DISPATCHER.init(device->data->vkDevice);

	//-----------------------------------------------------------
	// create nvrhi device

	auto instExtensions = VulkanInstance::_getVkInstanceExtensions();
	nvrhi::vulkan::DeviceDesc nvDeviceDesc;
	nvDeviceDesc.instance							 = instance;
	nvDeviceDesc.physicalDevice				 = physicalDevice;
	nvDeviceDesc.device								 = device->data->vkDevice;
	nvDeviceDesc.graphicsQueue				 = device->data->queues[QueueFamily::Graphics];
	nvDeviceDesc.graphicsQueueIndex		 = device->data->queueFamilies[QueueFamily::Graphics];
	nvDeviceDesc.computeQueue					 = device->data->queues[QueueFamily::Compute];
	nvDeviceDesc.computeQueueIndex		 = device->data->queueFamilies[QueueFamily::Compute];
	nvDeviceDesc.transferQueue				 = device->data->queues[QueueFamily::Transfer];
	nvDeviceDesc.transferQueueIndex		 = device->data->queueFamilies[QueueFamily::Transfer];
	nvDeviceDesc.instanceExtensions		 = instExtensions.data();
	nvDeviceDesc.numInstanceExtensions = instExtensions.size();
	nvDeviceDesc.deviceExtensions			 = device->data->extensions.data();
	nvDeviceDesc.numDeviceExtensions	 = device->data->extensions.size();

	device->data->nvDevice = nvrhi::vulkan::createDevice(nvDeviceDesc);

	std::cout << "done" << std::endl;
	return device;
}

//-------------------------

void VulkanDevice::shutdown() {
	data->nvDevice = nullptr;

	if (data->vkDevice) {
		data->vkDevice.destroy();
	}
	data->vkDevice = nullptr;
}

//-------------------------

void VulkanDevice::waitIdle() {
	if(data->nvDevice)
		data->nvDevice->waitForIdle();
}

//-------------------------

RenderFrameContextHandle VulkanDevice::createFrameContext(const WindowHandle& window) {
	Util::Reference<VulkanFrameContext> context = new VulkanFrameContext(this, window);
	RenderFrameContextHandle rfc = context;
	return context->init() ? context : nullptr;
}

//-------------------------

bool VulkanDevice::isWindowRenderingSupported() const {
	return data->enabledSurfaceSupport;
}

//-------------------------

/*
void VulkanDevice::allocateBuffer(Util::BufferHandle buffer, const uint8_t* data) {
}

//-------------------------

void VulkanDevice::allocateImage(Util::ImageHandle image) {
}

//-------------------------

void VulkanDevice::releaseResource(InternalResource* resource) {
}
*/

//-------------------------

const vk::Instance& VulkanDevice::_getVkInstance() const {
	return VulkanInstance::_getVkInstance();
}

//-------------------------

const vk::PhysicalDevice& VulkanDevice::_getVkPhysicalDevice() const {
	return data->physicalDevice;
}

//-------------------------

const vk::Device& VulkanDevice::_getVkDevice() const {
	return data->vkDevice;
}

//-------------------------

nvrhi::vulkan::DeviceHandle VulkanDevice::_getNvDevice() const {
	return data->nvDevice;
}

//-------------------------

nvrhi::DeviceHandle VulkanDevice::_getInternalDevice() const {
	return data->nvDevice;
}

//-------------------------

const vk::Queue& VulkanDevice::_getVkQueue(QueueFamily queueFamily) const {
	return data->queues[queueFamily];
}

//-------------------------

int32_t VulkanDevice::_getVkQueueFamilyIndex(QueueFamily queueFamily) const {
	return data->queueFamilies[queueFamily];
}

//-------------------------

} // namespace Rendering
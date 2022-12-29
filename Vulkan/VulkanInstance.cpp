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
#include "VulkanInstance.h"

#include <Util/UI/UI.h>
#include <Util/Macros.h>

#define VULKAN_HPP_NO_EXCEPTIONS
#include <nvrhi/nvrhi.h>
#include <nvrhi/validation.h>
#include <nvrhi/vulkan.h>

#include <unordered_map>
#include <vector>
#include <iostream>

// Define the Vulkan dynamic dispatcher - this needs to occur in exactly one cpp file in the program.
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace Rendering {
namespace {

//-------------------------

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {
	if ((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) > 0) {
		Util::output(Util::OUTPUT_ERROR, pCallbackData->pMessage);
	} else if ((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) > 0) {
		Util::output(Util::OUTPUT_WARNING, pCallbackData->pMessage);
	} else if ((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) > 0) {
		Util::output(Util::OUTPUT_INFO, pCallbackData->pMessage);
	} else if ((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) > 0) {
		Util::output(Util::OUTPUT_DEBUG, pCallbackData->pMessage);
	}
	return (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) > 0;
}

//-------------------------

std::string decodeVendorId(uint32_t id) {
	switch (id) {
		case 0x1002u: return "AMD";
		case 0x1010u: return "ImgTec";
		case 0x10DEu: return "NVIDIA";
		case 0x13B5u: return "ARM";
		case 0x5143u: return "Qualcomm";
		case 0x8086u: return "INTEL";
		default: return vk::to_string(static_cast<vk::VendorId>(id));
	}
}

//-------------------------

} // namespace

//-------------------------

struct VulkanInstance::Internal {
	vk::Instance instance = nullptr;
	vk::DebugUtilsMessengerEXT debugMessenger = nullptr;
	std::vector<PhysicalDeviceInfo> physicalDeviceInfos;
	std::unordered_map<Util::StringIdentifier, vk::PhysicalDevice> physicalDevices;
	std::vector<const char*> layers;
	std::vector<const char*> extensions;
};

//-------------------------

Util::Reference<VulkanInstance> VulkanInstance::instance;

//-------------------------

VulkanInstance::VulkanInstance(const VulkanInstanceConfig& config) : config(config), data(std::make_unique<Internal>()) {

}

//-------------------------

VulkanInstance::~VulkanInstance() {
	if (data->instance) {
		if (data->debugMessenger) {
			data->instance.destroyDebugUtilsMessengerEXT(data->debugMessenger);
		}
		data->instance.destroy();
	}
}

//-------------------------

bool VulkanInstance::init(const VulkanInstanceConfig& config) {
	WARN_AND_RETURN_IF(instance, "A Vulkan instance is already initialized.", false);
	std::cout << "Creating Vulkan instance..." << std::endl;

	// init dynamic loader
	const vk::DynamicLoader dl;
	const PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
	VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

	// check extensions & layers
	std::vector<const char*> validationLayers;
	if (config.debug) {
		validationLayers.emplace_back("VK_LAYER_KHRONOS_validation");
	}
	std::transform(config.validationLayers.cbegin(), config.validationLayers.cend(), std::back_inserter(validationLayers), [](const std::string& layer) {
		return layer.c_str();
	});

	if (!validationLayers.empty()) {
		std::cout << "  Validation layers:" << std::endl;
		auto availableLayers = vk::enumerateInstanceLayerProperties();
		bool allLayersFound	 = true;
		for (auto& layer : validationLayers) {
			bool found = false;
			for (auto& p : availableLayers.value) {
				if (std::strcmp(layer, p.layerName) == 0) {
					found = true;
					break;
				}
			}
			allLayersFound &= found;
			std::cout << "    " << layer << " - " << (found ? "found" : "not found") << std::endl;
		}
		WARN_AND_RETURN_IF(!allLayersFound, "Failed to create instance. Missing validation layers", false);
	}

	auto instanceExptensions = Util::UI::getRequiredInstanceExtensions();
	std::transform(config.extensions.cbegin(), config.extensions.cend(), std::back_inserter(instanceExptensions), [](const std::string& ext) {
		return ext.c_str();
	});

	instanceExptensions.emplace_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	if (config.debug) {
		instanceExptensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	std::cout << "  Instance Extensions:" << std::endl;
	auto availableExt				= vk::enumerateInstanceExtensionProperties();
	bool allExtensionsFound = true;
	for (auto& ext : instanceExptensions) {
		bool found = false;
		for (auto& p : availableExt.value) {
			if (std::strcmp(ext, p.extensionName) == 0) {
				found = true;
				break;
			}
		}
		allExtensionsFound &= found;
		std::cout << "    " << ext << " - " << (found ? "found" : "not found") << std::endl;
	}
	WARN_AND_RETURN_IF(!allExtensionsFound, "Failed to create instance. Missing extensions", false);

	// Create instance
	{
		auto appInfo = vk::ApplicationInfo()
										.setApiVersion(VK_MAKE_VERSION(config.apiVersionMajor, config.apiVersionMinor, 0))
										.setPApplicationName(config.name.c_str())
										.setPEngineName("PADrend")
										.setEngineVersion(VK_MAKE_VERSION(2, 0, 0));

		auto instInfo = vk::InstanceCreateInfo()
											.setPApplicationInfo(&appInfo)
											.setPEnabledExtensionNames(instanceExptensions)
											.setPEnabledLayerNames(validationLayers);

		auto [result, vkInstance] = vk::createInstance(instInfo);
		WARN_AND_RETURN_IF(result != vk::Result::eSuccess, "Failed to create instance. " + vk::to_string(result), false);
		
		VULKAN_HPP_DEFAULT_DISPATCHER.init(vkInstance);

		instance = new VulkanInstance(config);
		instance->data->instance = vkInstance;
		instance->data->layers.swap(validationLayers);
		instance->data->extensions.swap(instanceExptensions);
	}

	// create debug messenger
	if (config.debug) {
		auto createInfo = vk::DebugUtilsMessengerCreateInfoEXT()
												.setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
												.setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
												.setPfnUserCallback(debugCallback);
		auto [result, debugMessenger] = instance->data->instance.createDebugUtilsMessengerEXT(createInfo);
		WARN_AND_RETURN_IF(result != vk::Result::eSuccess, "failed to set up debug messenger. " + vk::to_string(result), false);
		instance->data->debugMessenger = debugMessenger;
	}

	// Enumerate all attached physical devices.
	{
		auto [result, physicalDevices] = instance->data->instance.enumeratePhysicalDevices();
		WARN_AND_RETURN_IF(result != vk::Result::eSuccess || physicalDevices.empty(), "No vulkan physical devices found.", false);

		uint32_t index = 0;
		for (auto pd : physicalDevices) {
			auto properties = pd.getProperties();

			PhysicalDeviceInfo info;
			info.deviceName			 = std::string_view(properties.deviceName);
			info.deviceType			 = static_cast<DeviceType>(properties.deviceType);
			info.apiName				 = "Vulkan";
			info.apiVersionMajor = VK_VERSION_MAJOR(properties.apiVersion);
			info.apiVersionMinor = VK_VERSION_MINOR(properties.apiVersion);
			info.vendorName			 = decodeVendorId(properties.vendorID);

			vk::PhysicalDeviceProperties2 properties2;
			vk::PhysicalDeviceDriverProperties driver;
			std::tie(properties2, driver) = pd.getProperties2<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceDriverProperties>();
			info.driverName								= std::string_view(driver.driverName);
			info.driverName += " ";
			info.driverName += std::string_view(driver.driverInfo);

			info.deviceId = {info.deviceName + " - " + std::to_string(index++)};

			instance->data->physicalDevices.emplace(info.deviceId, pd);
			instance->data->physicalDeviceInfos.emplace_back(std::move(info));
	}

		// TODO: fill info with capabilities
	}

	std::cout << "done" << std::endl;
	return true;
}

//-------------------------

void VulkanInstance::shutdown() {
	if(!instance)
		return;
	std::cout << "Shutting down Vulkan..." << std::endl;

	if (instance->data->debugMessenger) {
		instance->data->instance.destroyDebugUtilsMessengerEXT(instance->data->debugMessenger);
		instance->data->debugMessenger = nullptr;
	}
	instance = nullptr;
	std::cout << "done" << std::endl;
}

//-------------------------

const std::vector<PhysicalDeviceInfo>& VulkanInstance::getPhysicalDevices() {
	static const std::vector<PhysicalDeviceInfo> nullDevices;
	return instance ? instance->data->physicalDeviceInfos : nullDevices;
}

//-------------------------

const PhysicalDeviceInfo& VulkanInstance::getPhysicalDevice(Util::StringIdentifier deviceId) {
	static const PhysicalDeviceInfo nullDeviceInfo{};
	if(!instance)
		return nullDeviceInfo;
	auto it = std::find_if(instance->data->physicalDeviceInfos.begin(), instance->data->physicalDeviceInfos.end(), [deviceId](const PhysicalDeviceInfo& info) {
		return info.deviceId == deviceId;
	});
	return it != instance->data->physicalDeviceInfos.end() ? *it : nullDeviceInfo;
}

//-------------------------

const vk::Instance& VulkanInstance::_getVkInstance() {
	static const vk::Instance nullInstance;
	return instance ? instance->data->instance : nullInstance;
}

//-------------------------

const vk::PhysicalDevice& VulkanInstance::_getVkPhysicalDevice(Util::StringIdentifier deviceId) {
	static const vk::PhysicalDevice nullDevice;
	if(!instance)
		return nullDevice;
	if(auto it = instance->data->physicalDevices.find(deviceId); it != instance->data->physicalDevices.end()) {
		return it->second;
	}
	return nullDevice;
}

//-------------------------

const std::vector<const char*>& VulkanInstance::_getVkInstanceExtensions() {
	static const std::vector<const char*> nullExt;
	return instance ? instance->data->extensions : nullExt;
}

//-------------------------

const std::vector<const char*>& VulkanInstance::_getVkValidationLayers() {
	static const std::vector<const char*> nullLayers;
	return instance ? instance->data->layers : nullLayers;
}

//-------------------------


} // Rendering
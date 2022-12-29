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
#ifndef RENDERING_VULKAN_INSTANCE_H_
#define RENDERING_VULKAN_INSTANCE_H_

#include "../RenderDevice.h"

#include <memory>

namespace vk {
class Instance;
class PhysicalDevice;
} // vk

namespace Rendering {

//! Configurations for the Vulkan instance.
struct VulkanInstanceConfig {
	//! Name of the instance.
	std::string name;
	//! Requested major API version.
	uint32_t apiVersionMajor = 1;
	//! Requested minor API version.
	uint32_t apiVersionMinor = 3;
	//! Enable debug output (automatically adds necessary validation layers)
	bool debug = false;
	//! Enabled validation layers (requires debug=true).
	std::vector<std::string> validationLayers;
	//! Required instance extensions.
	std::vector<std::string> extensions;
};

//---------------------

//! Information about a physical rendering device.
struct PhysicalDeviceInfo {
	//! Identifier for the device.
	Util::StringIdentifier deviceId;
	//! Highest API major version this device supports.
	uint32_t apiVersionMajor;
	//! Highest API minor version this device supports.
	uint32_t apiVersionMinor;
	//! The type of the physical rendering device.
	DeviceType deviceType;
	//! API name (e.g., Vulkan)
	std::string apiName;
	//! Name of the device.
	std::string deviceName;
	//! Vendor name (e.g., NVIDIA).
	std::string vendorName;
	//! Driver name.
	std::string driverName;
};

//---------------------

/**
 * @brief Manages a singleton Vulkan instance.
 */
class VulkanInstance : public Util::ReferenceCounter<VulkanInstance> {
	PROVIDES_TYPE_NAME(VulkanInstance)
private:
	RENDERINGAPI VulkanInstance(const VulkanInstanceConfig& config);
public:
	RENDERINGAPI ~VulkanInstance();

	//! @name Instance
	// @{

	/**
	 * @brief Initializes the Vulkan instance.
	 * @note Needs to be called before creating a device or enumerating physical devices.
	 * @return true, when Vulkan was successfully initialized.
	 */
	RENDERINGAPI static bool init(const VulkanInstanceConfig& config);

	/**
	 * @brief Shuts down the Vulkan instance.
	 * @note Should be called before closing the application.
	 * All created vulkan devices should be destroyed before calling this.
	 */
	RENDERINGAPI static void shutdown();

	/**
	 * @brief Enumerates the available physical device.
	 * @note init() should be called before using this.
	 */
	RENDERINGAPI static const std::vector<PhysicalDeviceInfo>& getPhysicalDevices();

	/**
	 * @brief Get a specific physical device.
	 */
	RENDERINGAPI static const PhysicalDeviceInfo& getPhysicalDevice(Util::StringIdentifier deviceId);

	/**
	 * @brief Get the Vulkan instance configuration.
	 */
	static const VulkanInstanceConfig& getConfig() {
		static VulkanInstanceConfig emptyConfig{};
		return instance ? instance->config : emptyConfig;
	}

	static bool isValid() { return instance.isNotNull(); }

	static bool isDebugModeEnabled() { return instance ? instance->config.debug : false; }
	// @}
	

	//! @name Internal
	// @{
	
	RENDERINGAPI static const vk::Instance& _getVkInstance();
	
	RENDERINGAPI static const vk::PhysicalDevice& _getVkPhysicalDevice(Util::StringIdentifier deviceId);

	RENDERINGAPI static const std::vector<const char*>& _getVkInstanceExtensions();

	RENDERINGAPI static const std::vector<const char*>& _getVkValidationLayers();
	// @}
private:
	static Util::Reference<VulkanInstance> instance;
	const VulkanInstanceConfig config;
	// Uses pimpl idiom for internal storage.
	struct Internal;
	std::unique_ptr<Internal> data;
};

} // Rendering

#endif // RENDERING_VULKAN_INSTANCE_H_
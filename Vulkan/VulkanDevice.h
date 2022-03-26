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
#ifndef RENDERING_BACKEND_VULKANDEVICE_H_
#define RENDERING_BACKEND_VULKANDEVICE_H_

#include "VulkanInstance.h"

namespace nvrhi {
template <typename T>
class RefCountPtr;
namespace vulkan {
class IDevice;
using DeviceHandle = RefCountPtr<IDevice>;
} // vulkan
} // nvrhi

namespace vk {
class Device;
class Queue;
} // vk

namespace Rendering {
class VulkanDevice;
using VulkanDeviceHandle = Util::Reference<VulkanDevice>;

//---------------------

//! Configurations for the Vulkan device.
struct VulkanDeviceConfig {
	VulkanDeviceConfig(Util::StringIdentifier deviceId) : physicalDeviceId(deviceId), name(deviceId.toString()) {}
	VulkanDeviceConfig(const PhysicalDeviceInfo& physicalDevice) : physicalDeviceId(physicalDevice.deviceId), name(physicalDevice.deviceId.toString()) {}
	//! Physical device to create the render device from.
	Util::StringIdentifier physicalDeviceId;
	//! Name of the device (for debugging; does not have to match physical device name but is initialized as such).
	std::string name;
	//! Required device extensions.
	std::vector<std::string> extensions;
};

//---------------------

/**
 * @brief Represents a rendering device backed by the Vulkan rendering API.
 */
class VulkanDevice : public RenderDevice {
	PROVIDES_TYPE_NAME(VulkanDevice)
private:
	VulkanDevice(const VulkanDeviceConfig& config);
public:
	~VulkanDevice();

	//! @name Device
	// @{
	
	/**
	 * @brief Creates a Vulkan device. 
	 * @note You need to initialize the Vulkan instance first by calling VulkanInstance::init()
	 */
	RENDERINGAPI static VulkanDeviceHandle create(const VulkanDeviceConfig& config);

	/// ---|> [Device]
	RENDERINGAPI void shutdown() override;

	/// ---|> [Device]
	RENDERINGAPI void waitIdle() override;

	const VulkanDeviceConfig& getConfig() const { return config; }
	// @}
	
	//! @name Window & Surface rendering
	// @{

	/// ---|> [RenderDevice]
	RenderFrameContextHandle createFrameContext(const WindowHandle& window) override;
	
	/// ---|> [RenderDevice]
	bool isWindowRenderingSupported() const override;

	// @}

	//! @name Resources
	// @{
	
	/// ---|> [Device]
	RENDERINGAPI void allocateBuffer(Util::BufferHandle buffer, const uint8_t* data=nullptr) override;

	/// ---|> [Device]
	RENDERINGAPI void allocateImage(Util::ImageHandle image) override;

protected:
	struct InternalBufferResource;
	struct InternalImageResource;
	
	RENDERINGAPI void releaseResource(InternalResource* resource) override;
public:
	// @}

	//! @name Internal
	// @{
	
	const vk::Instance& _getVkInstance() const;

	const vk::PhysicalDevice& _getVkPhysicalDevice() const;

	const vk::Device& _getVkDevice() const;

	nvrhi::vulkan::DeviceHandle _getNvDevice() const;
	
	const vk::Queue& _getVkQueue(QueueFamily queueFamily) const;
	
	int32_t _getVkQueueFamilyIndex(QueueFamily queueFamily) const;

	// @}
private:
	const VulkanDeviceConfig config;
	// Uses pimpl idiom for internal storage.
	struct Internal;
	std::unique_ptr<Internal> data;
};

} // Rendering

#endif // RENDERING_BACKEND_VULKANDEVICE_H_
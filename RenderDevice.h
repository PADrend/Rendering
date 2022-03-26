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
#ifndef RENDERING_RENDERDEVICE_H_
#define RENDERING_RENDERDEVICE_H_

#include "RenderFrameContext.h"

#include <Util/Devices/Device.h>
#include <Util/Resources/Buffer.h>
#include <Util/Resources/Image.h>
#include <Util/Macros.h>

namespace Rendering {

//! Type of a rendering device.
enum class DeviceType : uint8_t {
	Other = 0,
	IntegratedGpu = 1,
	DiscreteGpu = 2,
	VirtualGpu = 3,
	Cpu = 4
};

//! Returns a string representation of DeviceType.
RENDERINGAPI std::string toString( DeviceType value );

//! Type of a rendering queue.
enum class QueueFamily : uint8_t {
	None = 0,
	Transfer = 1 << 0,
	Compute = 1 << 1,
	Graphics = 1 << 2,
	Present = 1 << 3,
};
DEFINE_BIT_OPERATORS(QueueFamily)

//! Returns a string representation of QueueFamily.
RENDERINGAPI std::string toString( QueueFamily value );

/**
 * @brief Represents a rendering device (GPU)
 * Manages resources & command submission to the the GPU.
 */
class RenderDevice : public Util::Device {
	PROVIDES_TYPE_NAME(RenderDevice)
public:
	/// ---|> [Device]
	//RENDERINGAPI void shutdown() override;

	
	//! @name Window rendering
	// @{

	/**
	 * @brief Create a render frame context for rendering to a window.
	 */
	virtual RenderFrameContextHandle createFrameContext(const WindowHandle& window) { return nullptr; };

	//! Returns if the device can render to a window surface
	virtual bool isWindowRenderingSupported() const { return false; }
public:
	// @}

	//! @name Resources
	// @{
	
	//! allocates memory for the given buffer and optionally initializes it with data.
	virtual void allocateBuffer(Util::BufferHandle buffer, const uint8_t* data=nullptr) = 0;

	//! Allocates memory for the given image.
	virtual void allocateImage(Util::ImageHandle image) = 0;

	// @}

};

using RenderDeviceHandle = Util::Reference<RenderDevice>;

} // Rendering

#endif // RENDERING_RENDERDEVICE_H_
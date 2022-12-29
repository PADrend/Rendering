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
#ifndef RENDERING_VULKAN_FRAMECONTEXT_H_
#define RENDERING_VULKAN_FRAMECONTEXT_H_

#include "../RenderFrameContext.h"

#include <memory>

namespace Rendering {
class VulkanDevice;
using VulkanDeviceHandle = Util::Reference<VulkanDevice>;

class VulkanFrameContext : public RenderFrameContext {
	PROVIDES_TYPE_NAME(VulkanFrameContext)
protected:
	RENDERINGAPI VulkanFrameContext(const VulkanDeviceHandle& device, const WindowHandle& handle);
public:
	RENDERINGAPI ~VulkanFrameContext();

	/// ---|> [RenderFrameContext]
	RENDERINGAPI void refresh() override;

	/// ---|> [RenderFrameContext]
	RENDERINGAPI void beginFrame() override;

	/// ---|> [RenderFrameContext]
	RENDERINGAPI void endFrame() override;
	
	/// ---|> [RenderFrameContext]
	RENDERINGAPI nvrhi::TextureHandle getCurrentSwapchainImage() const override;
	
	/// ---|> [RenderFrameContext]
	RENDERINGAPI nvrhi::FramebufferHandle getCurrentFramebuffer() const override;
private:
	friend class VulkanDevice;
	// Uses pimpl idiom for internal storage.
	struct Internal;
	std::unique_ptr<Internal> data;

	bool init();
	void destroySwapChain();
	bool recreateSwapChain();
};

} // Rendering

#endif // RENDERING_VULKAN_FRAMECONTEXT_H_
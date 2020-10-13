/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "../Common.h"
#include "../Device.h"
#include "../ImageView.h"
#include "../../State/PipelineState.h"
#include "../../FBO.h"
#include "../../Texture/Texture.h"

#include <Util/Graphics/Color.h>
#include <Util/Macros.h>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#include <vector>

namespace Rendering {

// defined in VkUtils.cpp
vk::Format getVkFormat(const InternalFormat& format);
vk::ImageLayout getVkImageLayout(const ResourceUsage& usage);

//---------------

ApiBaseHandle::Ref createRenderPassHandle(Device* device, const FramebufferFormat& state, bool clearColor, bool clearDepth, bool clearStencil, const std::vector<ResourceUsage>& lastColorUsages, ResourceUsage lastDepthUsage) {
	vk::Device vkDevice(device->getApiHandle());
	
	// Bind color buffers	
	uint32_t layerCount = 0;
	uint32_t attachmentCount = 0;
	std::vector<vk::AttachmentDescription> attachmentDescs;
	std::vector<vk::AttachmentReference> attachmentRefs(state.getColorAttachmentCount()+1, {VK_ATTACHMENT_UNUSED, vk::ImageLayout::eUndefined});
	
	for(uint32_t i=0; i<state.getColorAttachmentCount(); ++i) {
		auto& attachment = state.getColorAttachment(i);
		if(attachment.samples > 0 && attachment.format != InternalFormat::Unknown) {
			vk::ImageLayout srcLayout = (clearColor || i >= lastColorUsages.size()) ? vk::ImageLayout::eUndefined : getVkImageLayout(lastColorUsages[i]);
			vk::AttachmentLoadOp loadOp = (srcLayout == vk::ImageLayout::eUndefined) ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad;

			// Init color attachment descriptions
			attachmentDescs.emplace_back(vk::AttachmentDescriptionFlags{},
				static_cast<vk::Format>(getVkFormat(attachment.format)),
				static_cast<vk::SampleCountFlagBits>(attachment.samples),
				loadOp, vk::AttachmentStoreOp::eStore,
				vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
				srcLayout, vk::ImageLayout::eColorAttachmentOptimal
			);
			attachmentRefs[i] = {attachmentCount++, vk::ImageLayout::eColorAttachmentOptimal};
		}
	}
	
	// Bind depth buffer
	if(state.hasDepthStencilAttachment()) {
		auto& attachment = state.getDepthStencilAttachment();
		vk::ImageLayout srcLayout = (clearDepth && clearStencil) ? vk::ImageLayout::eUndefined : getVkImageLayout(lastDepthUsage);
		vk::AttachmentLoadOp depthLoadOp = (srcLayout == vk::ImageLayout::eUndefined || clearDepth) ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad;
		vk::AttachmentLoadOp stencilLoadOp = (srcLayout == vk::ImageLayout::eUndefined || clearStencil) ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad;
		
		// Init depth attachment descriptions.
		attachmentDescs.emplace_back(vk::AttachmentDescriptionFlags{},
			static_cast<vk::Format>(getVkFormat(attachment.format)),
			static_cast<vk::SampleCountFlagBits>(attachment.samples),
			depthLoadOp, vk::AttachmentStoreOp::eStore,
			stencilLoadOp, vk::AttachmentStoreOp::eStore,
			srcLayout, vk::ImageLayout::eDepthStencilAttachmentOptimal
		);
		attachmentRefs.back() = {attachmentCount++, vk::ImageLayout::eDepthStencilAttachmentOptimal};
	}
	
	// Init Subpass info
	vk::SubpassDescription subpassDesc = {};
	subpassDesc.colorAttachmentCount = state.getColorAttachmentCount();
	subpassDesc.pColorAttachments = attachmentRefs.data();
	if(state.hasDepthStencilAttachment()) {
		subpassDesc.pDepthStencilAttachment = &attachmentRefs.back();
	}
	std::vector<vk::SubpassDescription> subpassDescs{subpassDesc};
	// Multiple subpasses are currently not supported.

	// Create render pass
	auto pass = vkDevice.createRenderPass({{},
		static_cast<uint32_t>(attachmentDescs.size()), attachmentDescs.data(),
		static_cast<uint32_t>(subpassDescs.size()), subpassDescs.data()
	});
	return RenderPassHandle::create(pass, vkDevice).get();
}

//---------------

ApiBaseHandle::Ref createFramebufferHandle(Device* device, FBO* fbo, VkRenderPass renderpass) {
	vk::Device vkDevice(device->getApiHandle());
	if(!fbo || !renderpass || !fbo->isValid())
		return nullptr;
	
	uint32_t layerCount = 0;
	std::vector<vk::ImageView> attachments;

	for(const auto& attachment : fbo->getColorAttachments()) {
		if(attachment) {
			if(!attachment->isValid())
				attachment->upload();
			WARN_AND_RETURN_IF(layerCount != 0 && layerCount != attachment->getImageView()->getLayerCount(), "Framebuffer: Invalid layer count", nullptr);
			layerCount = attachment->getImageView()->getLayerCount();
			attachments.emplace_back(attachment->getImageView()->getApiHandle());
		}
	}
	
	const auto& depthStencilAttachment = fbo->getDepthStencilAttachment();
	if(depthStencilAttachment) {
		if(!depthStencilAttachment->isValid())
			depthStencilAttachment->upload();

		WARN_AND_RETURN_IF(layerCount != 0 && layerCount != depthStencilAttachment->getImageView()->getLayerCount(), "Framebuffer: Invalid layer count", nullptr);
		if(layerCount == 0)
			layerCount = depthStencilAttachment->getImageView()->getLayerCount();
		attachments.emplace_back(depthStencilAttachment->getImageView()->getApiHandle());
	}

	auto framebuffer = vkDevice.createFramebuffer({{},
		renderpass,
		static_cast<uint32_t>(attachments.size()), attachments.data(),
		!attachments.empty() ? fbo->getWidth() : 1, 
		!attachments.empty() ? fbo->getHeight() : 1,
		!attachments.empty() ? layerCount : 1
	});
	return FramebufferHandle::create(framebuffer, vkDevice).get();
}

} /* Rendering */
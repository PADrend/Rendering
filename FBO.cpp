/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	Copyright (C) 2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "FBO.h"
#include "Core/ApiHandles.h"
#include "Core/Device.h"
#include "Texture/Texture.h"
#include "Core/ImageStorage.h"
//#include "Texture/TextureView.h"
#include "GLHeader.h"

#include <Util/Macros.h>

#include <vulkan/vulkan.hpp>

#include <stdexcept>

namespace Rendering {

//=========================================================================

static FBO::Attachment&& createAttachment(const TextureRef& texture, uint32_t mipLevel, uint32_t baseLayer, uint32_t layerCount) {
	FBO::Attachment attachment{};
	attachment.mipLevel = mipLevel;
	attachment.baseLayer = baseLayer;
	attachment.layerCount = layerCount == 0 ? texture->getNumLayers() : layerCount;

	if(!texture || !texture->getImage())
		return std::move(attachment);

	vk::Device device(texture->getImage()->getApiHandle());
	vk::Image image(texture->getImage()->getApiHandle());

	auto type = vk::ImageViewType::e2D;
	auto format = vk::Format::eB8G8R8A8Unorm;
	
	auto view = device.createImageView({{}, 
		image, type, format, {},
		{ vk::ImageAspectFlagBits::eColor,
			attachment.mipLevel, 1,
			attachment.baseLayer, attachment.layerCount
		}
	});
	attachment.texture = texture;
	attachment.view = std::move(ImageViewHandle(view, device));	
	return std::move(attachment);
}

//-----------------

FBO::Ref FBO::create(const DeviceRef& device) {
	return new FBO(device);
}

//-----------------

FBO::FBO(const DeviceRef& device) : device(device) {
	colorAttachments.resize(device->getMaxFramebufferAttachments());
}

//-----------------

FBO::~FBO() = default;

//-----------------

void FBO::attachColorTexture(const TextureRef& texture, uint32_t index, uint32_t mipLevel, uint32_t baseLayer, uint32_t layerCount) {
	if(!texture) {
		detachColorTexture(index);
		return;
	}
	if(index >= colorAttachments.size()) {
		WARN("FBO: invalid attachment index " + std::to_string(index) + ". Maximum number of attachments is " + std::to_string(colorAttachments.size()) + ".");
		return;
	}
	colorAttachments[index] = std::move(createAttachment(texture, mipLevel, baseLayer, layerCount));
	width = texture->getWidth();
	height = texture->getHeight();
}

//-----------------

void FBO::detachColorTexture(uint32_t index) {
	if(index < colorAttachments.size()) {
		isValid = false;
		colorAttachments[index].texture = nullptr;
		colorAttachments[index].view = nullptr;
	}
}

//-----------------

void FBO::attachDepthStencilTexture(const TextureRef& texture, uint32_t mipLevel, uint32_t baseLayer, uint32_t layerCount) {
	if(!texture) {
		detachDepthStencilTexture();
		return;
	}
	depthStencilAttachment = std::move(createAttachment(texture, mipLevel, baseLayer, layerCount));
}

//-----------------

void FBO::detachDepthStencilTexture() {
	depthStencilAttachment.texture = nullptr;
	depthStencilAttachment.view = nullptr;
}

//-----------------

const FBO::Attachment& FBO::getColorTexture(uint32_t index) const {
	static Attachment nullAttachment{};
	if(index >= colorAttachments.size())
		return nullAttachment;
	return colorAttachments[index];
}

//-----------------

const FBO::Attachment& FBO::getDepthStencilTexture() const {
	return depthStencilAttachment;
}

//-----------------

bool FBO::isComplete() {
	if(!isValid) {
		if(!validate()) return false;
		init();
	}
	return true;
}

//-----------------

const std::string FBO::getStatusMessage() {
	return "";
}

//-----------------

bool FBO::validate() {
	
	return true;
}

//-----------------

void FBO::init() {
	vk::Device vkDevice(device->getApiHandle());
	
	// Bind color buffers	
	uint32_t layerCount = 0;
	uint32_t attachmentCount = 0;
	std::vector<vk::ImageView> attachments;
	std::vector<vk::AttachmentDescription> attachmentDescs;
	std::vector<vk::AttachmentReference> attachmentRefs(colorAttachments.size()+1, {VK_ATTACHMENT_UNUSED, vk::ImageLayout::eUndefined});
	
	for(uint32_t i=0; i<colorAttachments.size(); ++i) {
		auto& attachment = colorAttachments[i];
		if(attachment.view) {
			assert(layerCount == 0 || layerCount == colorAttachments[i].layerCount);
			layerCount = attachment.layerCount;
			attachments.emplace_back(attachment.view);
			
			// Init color attachment descriptions
			vk::Format format = vk::Format::eB8G8R8A8Unorm;
			attachmentDescs.emplace_back(vk::AttachmentDescriptionFlags{},
				format, vk::SampleCountFlagBits::e1,
				vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore,
				vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
				vk::ImageLayout::eColorAttachmentOptimal,
				vk::ImageLayout::eColorAttachmentOptimal
			);
			
			attachmentRefs[i] = {attachmentCount++, vk::ImageLayout::eColorAttachmentOptimal};
		}
	}
	bool hasColor = attachmentCount > 0;
	bool hasDepth = depthStencilAttachment.view;
	
	// Bind depth buffer
	if(hasDepth) {
		assert(layerCount == 0 || layerCount == depthStencilAttachment.layerCount);
		if(layerCount == 0) layerCount = depthStencilAttachment.layerCount;
		attachments.emplace_back(depthStencilAttachment.view);
		
		// Init depth attachment descriptions. No need to attach if the texture is null
		vk::Format format = vk::Format::eD24UnormS8Uint;
		attachmentDescs.emplace_back(vk::AttachmentDescriptionFlags{},
			format, vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore,
			vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore,
			vk::ImageLayout::eDepthStencilAttachmentOptimal,
			vk::ImageLayout::eDepthStencilAttachmentOptimal
		);
		attachmentRefs.back() = {attachmentCount++, vk::ImageLayout::eDepthStencilAttachmentOptimal};
	}
	
	// Init Subpass info
	std::vector<vk::SubpassDescription> subpassDescs;

	vk::SubpassDescription subpassDesc = {};
	if(hasColor) {
		subpassDesc.colorAttachmentCount = colorAttachments.size();
		subpassDesc.pColorAttachments = attachmentRefs.data();
	}
	
	// Depth
	if(hasDepth) {
		subpassDesc.pDepthStencilAttachment = &attachmentRefs.back();
	}	
	subpassDescs.emplace_back(subpassDesc);

	// Create render pass
	auto pass = vkDevice.createRenderPass({{},
		static_cast<uint32_t>(attachmentDescs.size()), attachmentDescs.data(),
		static_cast<uint32_t>(subpassDescs.size()), subpassDescs.data()
	});
	renderPass = std::move(RenderPassHandle(pass, vkDevice));

	// Framebuffer
	auto frameBuffer = vkDevice.createFramebuffer({{},
		pass,
		attachmentCount, attachments.data(),
		attachmentCount ? getWidth() : 1, 
		attachmentCount ? getHeight() : 1,
		attachmentCount ? layerCount : 1
	});
	handle = std::move(FramebufferHandle(frameBuffer, vkDevice));
}

//=========================================================================
// deprecated

FBO::FBO() : FBO(Device::getDefault()) { }

void FBO::attachTexture(RenderingContext & context,GLenum attachmentPoint,Texture * texture,uint32_t level,int32_t layer) {
	if(attachmentPoint == GL_DEPTH_ATTACHMENT || attachmentPoint == GL_STENCIL_ATTACHMENT)
		attachDepthStencilTexture(texture, level, layer < 0 ? 0 : layer, layer < 0 ? 0 : 1);
	else if(attachmentPoint >= GL_COLOR_ATTACHMENT0) {
		attachColorTexture(texture, attachmentPoint-GL_COLOR_ATTACHMENT0, level, layer < 0 ? 0 : layer, layer < 0 ? 0 : 1);
	}
}
void FBO::attachColorTexture(RenderingContext & context, Texture * t, uint32_t colorBufferId,uint32_t level,int32_t layer) {
	attachTexture(context, GL_COLOR_ATTACHMENT0 + colorBufferId, t,level,layer);
}
void FBO::detachColorTexture(RenderingContext & context, uint32_t colorBufferId) {
	detachTexture(context, GL_COLOR_ATTACHMENT0 + colorBufferId);
}
void FBO::attachDepthStencilTexture(RenderingContext & context, Texture * t,uint32_t level,int32_t layer) {
	attachTexture(context, GL_DEPTH_ATTACHMENT, t,level,layer);
	attachTexture(context, GL_STENCIL_ATTACHMENT, t,level,layer);
}
void FBO::detachDepthStencilTexture(RenderingContext & context) {
	detachTexture(context, GL_DEPTH_ATTACHMENT);
	detachTexture(context, GL_STENCIL_ATTACHMENT);
}
void FBO::attachDepthTexture(RenderingContext & context, Texture * t,uint32_t level,int32_t layer) {
	attachTexture(context, GL_DEPTH_ATTACHMENT, t,level,layer);
}
void FBO::detachDepthTexture(RenderingContext & context) {
	detachTexture(context, GL_DEPTH_ATTACHMENT);
}

void FBO::setDrawBuffers(RenderingContext & context, uint32_t number) {
}

void FBO::blitToScreen(RenderingContext & context, const Geometry::Rect_i& srcRect, const Geometry::Rect_i& tgtRect) {
}

}

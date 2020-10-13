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
#include "Core/ImageView.h"
#include "Core/Sampler.h"
#include "GLHeader.h"

#include <Util/Macros.h>
#include <Util/Utils.h>

#include <vulkan/vulkan.hpp>

#include <stdexcept>

namespace Rendering {

vk::Format getVkFormat(const InternalFormat& format);
vk::ImageLayout getVkImageLayout(const ResourceUsage& usage);

//-----------------

FBO::Ref FBO::create(const DeviceRef& device) {
	return new FBO(device);
}

//-----------------

FBO::FBO(const DeviceRef& device) : device(device) {
	colorAttachments.resize(device->getMaxFramebufferAttachments(), {});
}

//-----------------

FBO::~FBO() = default;

//-----------------

void FBO::attachColorTexture(const TextureRef& texture, uint32_t index) {
	if(index >= colorAttachments.size()) {
		WARN("FBO: invalid attachment index " + std::to_string(index) + ". Maximum number of attachments is " + std::to_string(colorAttachments.size()) + ".");
		return;
	}
	if(!texture) {
		detachColorTexture(index);
		return;
	}
	colorAttachments[index] = texture;
	width = texture->getWidth();
	height = texture->getHeight();
	invalidate();
}

//-----------------

void FBO::attachColorTexture(const ImageViewRef& view, uint32_t index) {
	if(index >= colorAttachments.size()) {
		WARN("FBO: invalid attachment index " + std::to_string(index) + ". Maximum number of attachments is " + std::to_string(colorAttachments.size()) + ".");
		return;
	}
	if(!view) {
		detachColorTexture(index);
		return;
	}
	attachColorTexture(Texture::create(device.get(), view), index);
}

//-----------------

void FBO::attachColorTexture(const ImageStorageRef& image, uint32_t index, uint32_t mipLevel, uint32_t baseLayer, uint32_t layerCount) {
	if(index >= colorAttachments.size()) {
		WARN("FBO: invalid attachment index " + std::to_string(index) + ". Maximum number of attachments is " + std::to_string(colorAttachments.size()) + ".");
		return;
	}
	if(!image) {
		detachColorTexture(index);
		return;
	}
	attachColorTexture(ImageView::create(image, {image->getType(), mipLevel, 1u, baseLayer, layerCount}), index);
}

//-----------------

void FBO::detachColorTexture(uint32_t index) {
	if(index < colorAttachments.size()) {
		invalidate();
		colorAttachments[index] = nullptr;
	}
}

//-----------------


void FBO::attachDepthStencilTexture(const TextureRef& texture) {
	depthStencilAttachment = texture;
	if(texture) {
		width = texture->getWidth();
		height = texture->getHeight();
		invalidate();
	}
}

//-----------------

void FBO::attachDepthStencilTexture(const ImageViewRef& view) {
	if(!view) {
		detachDepthStencilTexture();
		return;
	}
	attachDepthStencilTexture(Texture::create(device.get(), view));
}

//-----------------

void FBO::attachDepthStencilTexture(const ImageStorageRef& image, uint32_t mipLevel, uint32_t baseLayer, uint32_t layerCount) {
	if(!image) {
		detachDepthStencilTexture();
		return;
	}
	attachDepthStencilTexture(ImageView::create(image, {image->getType(), mipLevel, 1u, baseLayer, layerCount}));
}

//-----------------

void FBO::detachDepthStencilTexture() {
	depthStencilAttachment = nullptr;
	invalidate();
}

//-----------------

const TextureRef& FBO::getColorTexture(uint32_t index) const {
	static TextureRef nullRef;
	if(index >= colorAttachments.size())
		return nullRef;
	return colorAttachments[index];
}

//-----------------

const TextureRef& FBO::getDepthStencilTexture() const {
	return depthStencilAttachment;
}

//-----------------

const std::string FBO::getStatusMessage() const {
	return "";
}

//-----------------

bool FBO::validate() {
	if(!valid)
		init();
	valid = handle && renderPass;
	return valid;
}

//-----------------

void FBO::init() {
	vk::Device vkDevice(device->getApiHandle());
	hash = 0;
	
	// Bind color buffers	
	uint32_t layerCount = 0;
	uint32_t attachmentCount = 0;
	std::vector<vk::ImageView> attachments;
	std::vector<vk::AttachmentDescription> attachmentDescs;
	std::vector<vk::AttachmentReference> attachmentRefs(colorAttachments.size()+1, {VK_ATTACHMENT_UNUSED, vk::ImageLayout::eUndefined});
	
	for(uint32_t i=0; i<colorAttachments.size(); ++i) {
		auto& attachment = colorAttachments[i];
		if(attachment) {
			if(!attachment->isValid()) attachment->upload();

			assert(layerCount == 0 || layerCount == attachment->getImageView()->getLayerCount());
			layerCount = attachment->getImageView()->getLayerCount();
			attachments.emplace_back(attachment->getImageView()->getApiHandle());
			const auto& format = attachment->getFormat();
			auto lastLayout = getVkImageLayout(attachment->getImageView()->getLastUsage());

			// Init color attachment descriptions
			attachmentDescs.emplace_back(vk::AttachmentDescriptionFlags{},
				static_cast<vk::Format>(getVkFormat(format.pixelFormat)),
				static_cast<vk::SampleCountFlagBits>(format.samples),
				lastLayout == vk::ImageLayout::eUndefined ? vk::AttachmentLoadOp::eDontCare : vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore,
				vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
				lastLayout,
				vk::ImageLayout::ePresentSrcKHR
			);
			
			attachmentRefs[i] = {attachmentCount++, vk::ImageLayout::eColorAttachmentOptimal};

			Util::hash_combine(hash, attachmentCount);
			Util::hash_combine(hash, vk::ImageLayout::eColorAttachmentOptimal);
			Util::hash_combine(hash, format);
		}
	}
	bool hasColor = attachmentCount > 0;
	bool hasDepth = depthStencilAttachment;
	
	// Bind depth buffer
	if(hasDepth) {
		if(!depthStencilAttachment->isValid()) depthStencilAttachment->upload();

		assert(layerCount == 0 || layerCount == depthStencilAttachment->getImageView()->getLayerCount());
		if(layerCount == 0) layerCount = depthStencilAttachment->getImageView()->getLayerCount();
		attachments.emplace_back(depthStencilAttachment->getImageView()->getApiHandle());
		const auto& format = depthStencilAttachment->getFormat();
		auto lastLayout = getVkImageLayout(depthStencilAttachment->getImageView()->getLastUsage());
		
		// Init depth attachment descriptions. No need to attach if the texture is null
		attachmentDescs.emplace_back(vk::AttachmentDescriptionFlags{},
			static_cast<vk::Format>(getVkFormat(format.pixelFormat)),
			static_cast<vk::SampleCountFlagBits>(format.samples),
			lastLayout == vk::ImageLayout::eUndefined ? vk::AttachmentLoadOp::eDontCare : vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore,
			vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore,
			lastLayout,
			vk::ImageLayout::eDepthStencilAttachmentOptimal
		);
		attachmentRefs.back() = {attachmentCount++, vk::ImageLayout::eDepthStencilAttachmentOptimal};

		Util::hash_combine(hash, attachmentCount);
		Util::hash_combine(hash, vk::ImageLayout::eDepthStencilAttachmentOptimal);
		Util::hash_combine(hash, format);
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
	renderPass = RenderPassHandle::create(pass, vkDevice);

	// Framebuffer
	auto frameBuffer = vkDevice.createFramebuffer({{},
		pass,
		attachmentCount, attachments.data(),
		attachmentCount ? getWidth() : 1, 
		attachmentCount ? getHeight() : 1,
		attachmentCount ? layerCount : 1
	});
	handle = FramebufferHandle::create(frameBuffer, vkDevice);
}

//=========================================================================
// deprecated

FBO::FBO() : FBO(Device::getDefault()) { }

void FBO::attachTexture(RenderingContext & context,uint32_t attachmentPoint,Texture * texture,uint32_t level,int32_t layer) {
	if(attachmentPoint == GL_DEPTH_ATTACHMENT || attachmentPoint == GL_STENCIL_ATTACHMENT) {
		attachDepthStencilTexture(context, texture, level, layer);
	} else if(attachmentPoint >= GL_COLOR_ATTACHMENT0) {
		attachColorTexture(context, texture, attachmentPoint, level, layer);
	}
}
void FBO::detachTexture(RenderingContext & context, uint32_t attachmentPoint) { 
	attachTexture(context,attachmentPoint,nullptr,0,-1); 
}
void FBO::attachColorTexture(RenderingContext & context, Texture * texture, uint32_t colorBufferId,uint32_t level,int32_t layer) {
	if(!texture) {
		detachColorTexture(colorBufferId);
		return;
	} else if(!texture->isValid())
		texture->upload();

	uint32_t baseLayer = layer < 0 ? 0 : layer;
	uint32_t layerCount = layer < 0 ? 0 : 1;
	auto& view = texture->getImageView();
	if(view->getLayer() == baseLayer && view->getLayerCount() == layerCount && view->getMipLevel() == level) {
		attachColorTexture(texture);
	} else {
		attachColorTexture(view);
	}
}
void FBO::detachColorTexture(RenderingContext & context, uint32_t colorBufferId) {
	detachColorTexture(colorBufferId);
}
void FBO::attachDepthStencilTexture(RenderingContext & context, Texture * texture,uint32_t level,int32_t layer) {
	if(!texture) {
		detachDepthStencilTexture();
		return;
	} else if(!texture->isValid())
		texture->upload();

	uint32_t baseLayer = layer < 0 ? 0 : layer;
	uint32_t layerCount = layer < 0 ? 0 : 1;
	auto& view = texture->getImageView();
	if(view->getLayer() == baseLayer && view->getLayerCount() == layerCount && view->getMipLevel() == level) {
		attachDepthStencilTexture(texture);
	} else {
		attachDepthStencilTexture(view);
	}
}

void FBO::detachDepthStencilTexture(RenderingContext & context) {
	detachDepthStencilTexture();
}
void FBO::attachDepthTexture(RenderingContext & context, Texture * t,uint32_t level,int32_t layer) {
	attachDepthStencilTexture(context, t,level,layer);
}
void FBO::detachDepthTexture(RenderingContext & context) {
	detachDepthStencilTexture();
}

void FBO::setDrawBuffers(RenderingContext & context, uint32_t number) {
}

void FBO::blitToScreen(RenderingContext & context, const Geometry::Rect_i& srcRect, const Geometry::Rect_i& tgtRect) {
}

}

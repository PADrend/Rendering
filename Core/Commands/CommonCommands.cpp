/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "CommonCommands.h"
#include "../Device.h"
#include "../CommandBuffer.h"
#include "../ResourceCache.h"
#include "../DescriptorPool.h"
#include "../ImageView.h"
#include "../ImageStorage.h"
#include "../Swapchain.h"
#include "../../Texture/Texture.h"
#include "../../FBO.h"
#include "../internal/VkUtils.h"

#include <Util/Macros.h>

namespace Rendering {

//--------------

ExecuteCommandBufferCommand::~ExecuteCommandBufferCommand() = default;

//--------------

bool ExecuteCommandBufferCommand::compile(CompileContext& context) {
	WARN_AND_RETURN_IF(!buffer, "Cannot execute secondary command buffer. Invalid command buffer.", false);
	CompileContext subContext(context);
	subContext.cmd = nullptr;
	WARN_AND_RETURN_IF(!buffer->compile(subContext), "Failed to compile secondary command buffer.", false);
	static_cast<vk::CommandBuffer>(context.cmd).executeCommands(static_cast<vk::CommandBuffer>(buffer->getApiHandle()));
	return true;
}

//--------------

BeginRenderPassCommand::~BeginRenderPassCommand() = default;

//--------------

BeginRenderPassCommand::BeginRenderPassCommand(const FBORef& fbo, std::vector<Util::Color4f> colors, float depthValue, uint32_t stencilValue, bool clearColor, bool clearDepth, bool clearStencil) :
	fbo(fbo), colors(colors), depthValue(depthValue), stencilValue(stencilValue), clearColor(clearColor), clearDepth(clearDepth), clearStencil(clearStencil){
}

//--------------

bool BeginRenderPassCommand::compile(CompileContext& context) {
	if(!fbo)
		fbo = context.device->getSwapchain()->getCurrentFBO();

	WARN_AND_RETURN_IF(!fbo || !fbo->isValid(), "Failed to start render pass. Invalid FBO.", false);
	std::vector<ResourceUsage> lastColorUsages;
	for(auto& att : fbo->getColorAttachments())
		lastColorUsages.emplace_back(att.isNotNull() ? att->getImage()->getLastUsage() : ResourceUsage::Undefined);
	ResourceUsage lastDepthUsage = fbo->getDepthStencilAttachment() ? fbo->getDepthStencilAttachment()->getImage()->getLastUsage() : ResourceUsage::Undefined;

	renderPass = context.resourceCache->createRenderPass(fbo, lastColorUsages, lastDepthUsage, clearColor, clearDepth, clearStencil);

	WARN_AND_RETURN_IF(!renderPass, "Failed to start render pass. Invalid render pass.", false);
	framebuffer = context.resourceCache->createFramebuffer(fbo, renderPass);
	WARN_AND_RETURN_IF(!framebuffer, "Failed to start render pass. Invalid framebuffer.", false);

	std::vector<vk::ClearValue> clearValues(fbo->getColorAttachmentCount(), vk::ClearColorValue{});
	for(uint32_t i=0; i<std::min<size_t>(clearValues.size(), colors.size()); ++i) {
		auto& c = colors[i];
		clearValues[i].color.setFloat32({c.r(), c.g(), c.b(), c.a()});
	}
	clearValues.emplace_back(vk::ClearDepthStencilValue(depthValue, stencilValue));

	static_cast<vk::CommandBuffer>(context.cmd).beginRenderPass({
		{renderPass}, {framebuffer},
		vk::Rect2D{ {0, 0}, {fbo->getWidth(), fbo->getHeight()} },
		static_cast<uint32_t>(clearValues.size()), clearValues.data()
	}, vk::SubpassContents::eInline);
	return true;
}

//--------------

bool EndRenderPassCommand::compile(CompileContext& context) {
	if(!fbo)
		fbo = context.device->getSwapchain()->getCurrentFBO();

	for(uint32_t i=0; i<fbo->getColorAttachmentCount(); ++i) {
		const auto& attachment = fbo->getColorAttachment(i);
		if(attachment)
			attachment->getImage()->_setLastUsage(ResourceUsage::RenderTarget);
	}
	const auto& depthAttachment = fbo->getDepthStencilAttachment();
	if(depthAttachment)
		depthAttachment->getImage()->_setLastUsage(ResourceUsage::DepthStencil);
	static_cast<vk::CommandBuffer>(context.cmd).endRenderPass();
	return true;
}

//--------------

bool PrepareForPresentCommand::compile(CompileContext& context) {
	auto fbo = context.device->getSwapchain()->getCurrentFBO();
	transferImageLayout(context.cmd, fbo->getColorAttachment()->getImageView(), ResourceUsage::Present);
	return true;
}

//--------------

PushConstantCommand::~PushConstantCommand() = default;

//--------------

bool PushConstantCommand::compile(CompileContext& context) {
	WARN_AND_RETURN_IF(constantData.size()+offset > context.device->getMaxPushConstantSize(), "Push constant size exceeds maximum size", false);
	pipelineLayout = context.resourceCache->createPipelineLayout(layout);
	WARN_AND_RETURN_IF(!pipelineLayout, "Failed to create pipeline layout for layout: " + toString(layout), false);
	
	vk::ShaderStageFlags stages{};
	for(auto& range : layout.getPushConstantRanges()) {
		if(offset >= range.offset && offset + constantData.size() <= range.offset + range.size) {
			stages |= getVkStageFlags(range.stages);
		}
	}
	if(stages) {
		static_cast<vk::CommandBuffer>(context.cmd).pushConstants({pipelineLayout}, stages, offset, static_cast<uint32_t>(constantData.size()), constantData.data());
	}
	return true;
}

ImageBarrierCommand::ImageBarrierCommand(const TextureRef& texture, ResourceUsage newUsage) :
	view(texture->getImageView()), image(nullptr), newUsage(newUsage) {}

//--------------

ImageBarrierCommand::~ImageBarrierCommand() = default;

//--------------

bool ImageBarrierCommand::compile(CompileContext& context) {
	WARN_AND_RETURN_IF(!image && !view, "Cannot create image barrier. Invalid image or image view.", false);
	if(view)
		transferImageLayout(context.cmd, view, newUsage);
	else
		transferImageLayout(context.cmd, image, newUsage);
	return true;
}

//--------------

bool DebugMarkerCommand::compile(CompileContext& context) {
	if(context.device->isDebugModeEnabled()) {
		vk::DebugUtilsLabelEXT label(name.c_str(), {color.r(), color.g(), color.b(), color.a()});
		switch(mode) {
			case Begin:
				static_cast<vk::CommandBuffer>(context.cmd).beginDebugUtilsLabelEXT(label);
				break;
			case End:
				static_cast<vk::CommandBuffer>(context.cmd).endDebugUtilsLabelEXT();
				break;
			case Insert:
				static_cast<vk::CommandBuffer>(context.cmd).insertDebugUtilsLabelEXT(label);
				break;
		}
	}
	return true;
}

//--------------

} /* Rendering */
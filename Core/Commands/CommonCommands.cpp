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
#include "../ImageView.h"
#include "../ImageStorage.h"
#include "../../Texture/Texture.h"
#include "../../FBO.h"

#include <Util/Macros.h>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

namespace Rendering {

// defined in internal/VkUtils.cpp
vk::ShaderStageFlags getVkStageFlags(const ShaderStage& stages);
vk::AccessFlags getVkAccessMask(const ResourceUsage& usage);
vk::PipelineStageFlags getVkPipelineStageMask(const ResourceUsage& usage, bool src);
vk::ImageLayout getVkImageLayout(const ResourceUsage& usage);

//--------------

ExecuteCommandBufferCommand::~ExecuteCommandBufferCommand() = default;

//--------------

bool ExecuteCommandBufferCommand::compile(CompileContext& context) {
	WARN_AND_RETURN_IF(!buffer, "Cannot execute secondary command buffer. Invalid command buffer.", false);
	WARN_AND_RETURN_IF(!buffer->compile(), "Failed to compile secondary command buffer.", false);
	static_cast<vk::CommandBuffer>(context.cmd).executeCommands(static_cast<vk::CommandBuffer>(buffer->getApiHandle()));
	return true;
}

//--------------

BeginRenderPassCommand::~BeginRenderPassCommand() = default;

//--------------

bool BeginRenderPassCommand::compile(CompileContext& context) {
	WARN_AND_RETURN_IF(!fbo || !fbo->isValid(), "Failed to start render pass. Invalid FBO.", false);
	auto renderPass = context.resourceCache->createRenderPass(fbo, clearColor, clearDepth, clearStencil);
	WARN_AND_RETURN_IF(!renderPass, "Failed to start render pass. Invalid render pass.", false);
	auto framebuffer = context.resourceCache->createFramebuffer(fbo, renderPass);
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
	static_cast<vk::CommandBuffer>(context.cmd).endRenderPass();
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

//--------------

ImageBarrierCommand::ImageBarrierCommand(const TextureRef& texture, ResourceUsage oldUsage, ResourceUsage newUsage) :
	view(texture->getImageView()), image(nullptr), oldUsage(oldUsage), newUsage(newUsage) {}

//--------------

ImageBarrierCommand::~ImageBarrierCommand() = default;

//--------------

bool ImageBarrierCommand::compile(CompileContext& context) {
	if(oldUsage == newUsage) return true;
	WARN_AND_RETURN_IF(!image && !view, "Cannot create image barrier. Invalid image or image view.", false);
	
	ImageStorageRef img = image ? image : view->getImage();
	const auto& format = img->getFormat();

	vk::ImageMemoryBarrier barrier{};
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.srcAccessMask = getVkAccessMask(oldUsage);
	barrier.dstAccessMask = getVkAccessMask(newUsage);
	barrier.oldLayout = getVkImageLayout(oldUsage);
	barrier.newLayout = getVkImageLayout(newUsage);
	barrier.image = img->getApiHandle();
	barrier.subresourceRange.aspectMask = isDepthStencilFormat(format) ? (vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil) : vk::ImageAspectFlagBits::eColor;
	if(view) {
		barrier.subresourceRange.baseArrayLayer = view->getLayer();
		barrier.subresourceRange.layerCount = view->getLayerCount();
		barrier.subresourceRange.baseMipLevel = view->getMipLevel();
		barrier.subresourceRange.levelCount = view->getMipLevelCount();
	} else {
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = format.layers;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = format.mipLevels;
	}

	static_cast<vk::CommandBuffer>(context.cmd).pipelineBarrier(
		getVkPipelineStageMask(oldUsage, true),
		getVkPipelineStageMask(newUsage, false),
		{}, {}, {}, {barrier}
	);
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
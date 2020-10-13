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

#include <Util/Macros.h>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

namespace Rendering {

// defined in internal/VkUtils.cpp
vk::ShaderStageFlags getVkStageFlags(const ShaderStage& stages);
void enqueueVkImageBarrier(const CommandBufferHandle& cmd, const ImageStorage::Ref& image, ResourceUsage newUsage);
void enqueueVkImageBarrier(const CommandBufferHandle& cmd, const ImageView::Ref& view, ResourceUsage newUsage);

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

PushConstantCommand::~PushConstantCommand() = default;

//--------------

bool PushConstantCommand::compile(CompileContext& context) {
	WARN_AND_RETURN_IF(constantData.size()+offset > context.device->getMaxPushConstantSize(), "Push constant size exceeds maximum size",);
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

ImageBarrierCommand::ImageBarrierCommand(const TextureRef& texture, ResourceUsage newUsage) : view(texture->getImageView()), image(nullptr), newUsage(newUsage) {}

//--------------

ImageBarrierCommand::~ImageBarrierCommand() = default;

//--------------

bool ImageBarrierCommand::compile(CompileContext& context) {
	if(view) {
		enqueueVkImageBarrier(context.cmd, view, newUsage);
	} else if(image) {
		enqueueVkImageBarrier(context.cmd, image, newUsage);
	} else {
		WARN("Cannot create image barrier. Invalid image or image view.");
		return false;
	}
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
				static_cast<vk::CommandBuffer>(context.cmd).endDebugUtilsLabelEXT(label);
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
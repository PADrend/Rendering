/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "BindCommands.h"
#include "../Device.h"
#include "../DescriptorPool.h"
#include "../ResourceCache.h"
#include "../../Buffer/BufferObject.h"
#include "../ImageView.h"
#include "../../Texture/Texture.h"
#include "../../Shader/Shader.h"
#include "../internal/VkUtils.h"

#include <Util/Macros.h>

namespace Rendering {

//--------------

BindPipelineCommand::~BindPipelineCommand() = default;

//--------------

bool BindPipelineCommand::compile(CompileContext& context) {
	vk::PipelineBindPoint vkBindPoint;
	switch(pipeline.getType()) {
		case PipelineType::Compute: vkBindPoint = vk::PipelineBindPoint::eCompute; break;
		case PipelineType::Graphics: vkBindPoint = vk::PipelineBindPoint::eGraphics; break;
	}
	pipelineHandle = context.resourceCache->createPipeline(pipeline, nullptr);
	WARN_AND_RETURN_IF(!pipelineHandle, "Cannot bind pipeline: Invalid pipeline.", false);
	static_cast<vk::CommandBuffer>(context.cmd).bindPipeline(vkBindPoint, {pipelineHandle});
	return true;
}

//--------------

BindSetCommand::~BindSetCommand() = default;

//--------------

bool BindSetCommand::compile(CompileContext& context) {
	if(!layout.hasLayoutSet(set))
		return true;

	descriptorSet = context.descriptorPool->requestDescriptorSet(layout.getLayoutSet(set), bindingSet);
	WARN_AND_RETURN_IF(!descriptorSet, "Failed to create descriptor set for binding set " + std::to_string(set), false);
	pipelineLayout = context.resourceCache->createPipelineLayout(layout);
	WARN_AND_RETURN_IF(!pipelineLayout, "Failed to create pipeline layout for layout: " + toString(layout), false);

	vk::PipelineLayout vkPipelineLayout(pipelineLayout);
	vk::PipelineBindPoint vkBindPoint;
	switch(bindingPoint) {
		case PipelineType::Compute: vkBindPoint = vk::PipelineBindPoint::eCompute; break;
		case PipelineType::Graphics: vkBindPoint = vk::PipelineBindPoint::eGraphics; break;
	}

	vk::CommandBuffer vkCmd(context.cmd);
	vk::DescriptorSet vkSet(descriptorSet->getApiHandle());
	vkCmd.bindDescriptorSets(vkBindPoint, vkPipelineLayout, set, {vkSet}, descriptorSet->getDynamicOffsets());
	return true;
}

//--------------

BindVertexBuffersCommand::~BindVertexBuffersCommand() = default;

//--------------

bool BindVertexBuffersCommand::compile(CompileContext& context) {
	std::vector<vk::Buffer> vkBuffers;
	std::vector<vk::DeviceSize> vkOffsets;
	for(auto& bo : buffers) {
		WARN_AND_RETURN_IF(!bo || !bo->isValid(), "Could not bind vertex buffer: Invalid buffer.", false);
		vkBuffers.emplace_back(bo->getApiHandle());
		vkOffsets.emplace_back(bo->getOffset());
	}
	static_cast<vk::CommandBuffer>(context.cmd).bindVertexBuffers(firstBinding, vkBuffers, vkOffsets);
	return true;
}

//--------------

BindIndexBufferCommand::~BindIndexBufferCommand() = default;

//--------------

bool BindIndexBufferCommand::compile(CompileContext& context) {
	WARN_AND_RETURN_IF(!buffer || !buffer->isValid(), "Could not bind index buffer: Invalid buffer.", false);
	static_cast<vk::CommandBuffer>(context.cmd).bindIndexBuffer({buffer->getApiHandle()}, buffer->getOffset(), vk::IndexType::eUint32);
	return true;
}

//--------------

} /* Rendering */
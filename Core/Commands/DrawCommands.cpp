/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "DrawCommands.h"
#include "../Device.h"
#include "../../Buffer/BufferObject.h"

#include <Util/Macros.h>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

namespace Rendering {

//--------------

bool DrawCommand::compile(CompileContext& context) {
	if(instanceCount==0) return true;
	static_cast<vk::CommandBuffer>(context.cmd).draw(vertexCount, instanceCount, firstVertex, firstInstance);
	return true;
}

//--------------

bool DrawIndexedCommand::compile(CompileContext& context) {
	if(instanceCount==0) return true;
	static_cast<vk::CommandBuffer>(context.cmd).drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	return true;
}

//--------------

DrawIndirectCommand::~DrawIndirectCommand() = default;

//--------------

bool DrawIndirectCommand::compile(CompileContext& context) {
	if(drawCount==0) return true;
	WARN_AND_RETURN_IF(!buffer->isValid(), "Cannot perform indirect draw. Buffer is not valid.", false);
	static_cast<vk::CommandBuffer>(context.cmd).drawIndirect({buffer->getApiHandle()}, offset, drawCount, stride);
	return true;
}

//--------------

DrawIndexedIndirectCommand::~DrawIndexedIndirectCommand() = default;

//--------------

bool DrawIndexedIndirectCommand::compile(CompileContext& context) {
	if(drawCount==0) return true;
	WARN_AND_RETURN_IF(!buffer->isValid(), "Cannot perform indirect draw. Buffer is not valid.", false);
	static_cast<vk::CommandBuffer>(context.cmd).drawIndexedIndirect({buffer->getApiHandle()}, offset, drawCount, stride);
	return true;
}
//--------------

bool ClearAttachmentsCommand::compile(CompileContext& context) {
	std::vector<vk::ClearAttachment> clearAttachments;
	if(clearColor) {
		for(uint32_t i=0; i<colors.size(); ++i) {
			const auto& c = colors[i];
			vk::ClearAttachment att{};
			att.clearValue.color.setFloat32({c.r(), c.g(), c.b(), c.a()});
			att.colorAttachment = i;
			att.aspectMask = vk::ImageAspectFlagBits::eColor;
			clearAttachments.emplace_back(att);
		}
	}

	if(clearDepth || clearStencil) {
		vk::ClearAttachment att{};
		if(clearDepth)
			att.aspectMask |= vk::ImageAspectFlagBits::eDepth;
		if(clearStencil)
			att.aspectMask |= vk::ImageAspectFlagBits::eStencil;
		att.clearValue.depthStencil.depth = depthValue;
		att.clearValue.depthStencil.stencil = stencilValue;
		clearAttachments.emplace_back(att);
	}

	vk::ClearRect clearRect;	
	clearRect.baseArrayLayer = 0;
	clearRect.layerCount = 1;
	clearRect.rect = vk::Rect2D{
		{rect.getX(), rect.getY()},
		{rect.getWidth(), rect.getHeight()}
	};
	static_cast<vk::CommandBuffer>(context.cmd).clearAttachments(clearAttachments, {clearRect});
	return true;
}

//--------------

} /* Rendering */
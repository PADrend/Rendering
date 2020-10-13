/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "DrawCommands.h"
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

} /* Rendering */
/*
	This file is part of the Rendering library.
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "CommandBuffer.h"
#include "CommandPool.h"
#include "Device.h"

#include <vulkan/vulkan.hpp>

namespace Rendering {

//-----------------

CommandBuffer::CommandBuffer(CommandPool* pool, bool primary) : pool(pool), primary(primary) { }

//-----------------

CommandBuffer::~CommandBuffer() {
	if(!handle) return;
	vk::Device vkDevice(handle);
	vk::CommandBuffer vkBuffer(handle);
	vk::CommandPool vkPool(pool->getApiHandle());
	vkDevice.freeCommandBuffers(vkPool, 1, &vkBuffer);
}

//-----------------

bool CommandBuffer::init() {
	vk::Device vkDevice(pool->getApiHandle());
	vk::CommandPool vkPool(pool->getApiHandle());

	auto buffers = vkDevice.allocateCommandBuffers({
		vkPool,
		primary ? vk::CommandBufferLevel::ePrimary : vk::CommandBufferLevel::eSecondary
	});

	if(buffers.empty() || !buffers.front())
		return false;

	handle = std::move(CommandBufferHandle(buffers.front(), vkDevice));

	return true;
}

//-----------------

void CommandBuffer::reset() {
	vk::CommandBuffer vkBuffer(handle);
	if(state == State::Recording)
		end();
	vkBuffer.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
}

//-----------------

} // namespace Rendering

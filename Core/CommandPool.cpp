/*
	This file is part of the Rendering library.
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "CommandPool.h"
#include "CommandBuffer.h"
#include "Device.h"
#include "../Shader/Shader.h"
#include "../FBO.h"

#include <Util/Macros.h>

#include <vulkan/vulkan.hpp>

namespace Rendering {

//-----------------

CommandPool::~CommandPool() = default;

//-----------------

CommandBuffer::Ref CommandPool::requestCommandBuffer(bool primary) {
	CommandBuffer::Ref obj;
	while(!active.empty() && active.front()->getState() == CommandBuffer::State::Free) {
		if(active.front()->isPrimary())
			freePrimary.emplace_back(std::move(active.front()));
		else
			freeSecondary.emplace_back(std::move(active.front()));
		active.pop_front();
	}
	auto& tmpFree = primary ? freePrimary : freeSecondary;
	if(tmpFree.empty()) {
		obj = new CommandBuffer(this, primary);
		if(!obj->init()) {
			WARN("CommandPool: Failed to initialize command buffer.");
			return nullptr;
		}
	} else {
		obj = tmpFree.back();
		tmpFree.pop_back();
		obj->reset();
	}
	active.emplace_back(obj);
	return obj;
}

//-----------------

CommandPool::CommandPool(const DeviceRef& device, uint32_t queueFamily) : device(device), queueFamily(queueFamily) {
	vk::Device vkDevice(device->getApiHandle());
	handle = std::move(CommandPoolHandle(vkDevice.createCommandPool({
		vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		queueFamily
	}), vkDevice));
}

//-----------------

} // namespace Rendering

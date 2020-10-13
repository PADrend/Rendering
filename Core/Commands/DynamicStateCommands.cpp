/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "DynamicStateCommands.h"

#include <Util/Macros.h>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

namespace Rendering {

//--------------

bool DynamicScissorCommand::compile(CompileContext& context) {
	std::vector<vk::Rect2D> rects;
	for(auto& scissor : scissors) {
		vk::Rect2D rect{};
		rect.extent.width = static_cast<uint32_t>(scissor.getWidth());
		rect.extent.height = static_cast<uint32_t>(scissor.getHeight());
		rect.offset.x = std::max(0, scissor.getX());
		rect.offset.y = std::max(0, scissor.getY());
		rects.emplace_back(std::move(rect));
	}
	static_cast<vk::CommandBuffer>(context.cmd).setScissor(firstScissor, rects);
	return true;
}

//--------------

} /* Rendering */
/*
	This file is part of the Rendering library.
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "Common.h"

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#define API_BASE_IMPL(HandleType) HandleType##Handle::~HandleType##Handle() { \
	if(handle && owner) \
		vkDestroy##HandleType(handle, nullptr); \
}

#define API_IMPL_NO_DESTR(HandleType) HandleType##Handle::~HandleType##Handle() = default;

#define API_IMPL(HandleType) HandleType##Handle::~HandleType##Handle() { \
	if(handle && owner) \
		vkDestroy##HandleType(parent, handle, nullptr); \
}

#define API_IMPL_KHR(HandleType) HandleType##Handle::~HandleType##Handle() { \
	if(handle && owner) \
		vkDestroy##HandleType##KHR(parent, handle, nullptr); \
}

namespace Rendering {
	
API_BASE_IMPL(Instance)
API_BASE_IMPL(Device)
API_IMPL_KHR(Surface)
API_IMPL_KHR(Swapchain)
API_IMPL_NO_DESTR(Queue)
API_IMPL(Fence)
API_IMPL(Image)
API_IMPL(ImageView)
API_IMPL(Framebuffer)
API_IMPL(RenderPass)
API_IMPL(CommandPool)
API_IMPL_NO_DESTR(CommandBuffer)
API_IMPL_NO_DESTR(Memory)
API_IMPL(Buffer)
API_IMPL(BufferView)
API_IMPL(Pipeline)
API_IMPL(ShaderModule)

AllocatorHandle::~AllocatorHandle() {
	if(handle) vmaDestroyAllocator(handle);
}

AllocationHandle::~AllocationHandle() {
	if(handle) vmaFreeMemory(parent, handle);
}

} /* Rendering */
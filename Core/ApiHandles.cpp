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

#define API_NO_DESTROY(HandleType) HandleType##Handle::~HandleType##Handle() = default;
#define API_DESTROY(HandleType, destroyFunction, ...) HandleType##Handle::~HandleType##Handle() { \
	if(handle && owner) destroyFunction(__VA_ARGS__); \
}

#define API_DEFAULT_DESTROY_NO_PARENT(HandleType) API_DESTROY(HandleType, vkDestroy##HandleType, handle, nullptr)
#define API_DEFAULT_DESTROY(HandleType) API_DESTROY(HandleType, vkDestroy##HandleType, parent, handle, nullptr)
#define API_DEFAULT_DESTROY_KHR(HandleType) API_DESTROY(HandleType, vkDestroy##HandleType##KHR, parent, handle, nullptr)

namespace Rendering {
	
API_DEFAULT_DESTROY_NO_PARENT(Instance)
API_DEFAULT_DESTROY_NO_PARENT(Device)
API_DEFAULT_DESTROY_KHR(Surface)
API_DEFAULT_DESTROY_KHR(Swapchain)
API_NO_DESTROY(Queue)
API_DEFAULT_DESTROY(Fence)
API_DEFAULT_DESTROY(Image)
API_DEFAULT_DESTROY(ImageView)
API_DEFAULT_DESTROY(Framebuffer)
API_DEFAULT_DESTROY(RenderPass)
API_DEFAULT_DESTROY(CommandPool)
API_NO_DESTROY(CommandBuffer)
API_NO_DESTROY(Memory)
API_DEFAULT_DESTROY(Buffer)
API_DEFAULT_DESTROY(BufferView)
API_DEFAULT_DESTROY(Pipeline)
API_DEFAULT_DESTROY(PipelineCache)
API_DEFAULT_DESTROY(PipelineLayout)
API_DEFAULT_DESTROY(ShaderModule)
API_DEFAULT_DESTROY(DescriptorSetLayout);
API_DEFAULT_DESTROY(DescriptorPool);
API_DEFAULT_DESTROY(Sampler)

API_DESTROY(DescriptorSet, vkFreeDescriptorSets, parent.first, parent.second, 1, &handle);
API_DESTROY(Allocator, vmaDestroyAllocator, handle);
API_DESTROY(Allocation, vmaFreeMemory, parent, handle);

} /* Rendering */
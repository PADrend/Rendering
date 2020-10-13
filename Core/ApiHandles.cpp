/*
	This file is part of the Rendering library.
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "Common.h"

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>

namespace Rendering {

template<> ApiHandle<VkInstance, VkNullHandle>::~ApiHandle() { if(handle) VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyInstance(handle, nullptr); }
template<> ApiHandle<VkDevice, VkPhysicalDevice>::~ApiHandle() { if(handle) VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyDevice(handle, nullptr); }
template<> ApiHandle<VkSurfaceKHR, VkInstance>::~ApiHandle() { if(handle) VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroySurfaceKHR(parent, handle, nullptr); }
template<> ApiHandle<VkSwapchainKHR, VkDevice>::~ApiHandle() { if(handle) VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroySwapchainKHR(parent, handle, nullptr); }
template<> ApiHandle<VkQueue, VkDevice>::~ApiHandle() { }
template<> ApiHandle<VkFence, VkDevice>::~ApiHandle() { if(handle) VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyFence(parent, handle, nullptr); }
template<> ApiHandle<VkSemaphore, VkDevice>::~ApiHandle() { if(handle) VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroySemaphore(parent, handle, nullptr); }
template<> ApiHandle<VkImage, VkDevice>::~ApiHandle() { if(handle) VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyImage(parent, handle, nullptr); }
template<> ApiHandle<VkImageView, VkDevice>::~ApiHandle() { if(handle) VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyImageView(parent, handle, nullptr); }
template<> ApiHandle<VkFramebuffer, VkDevice>::~ApiHandle() { if(handle) VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyFramebuffer(parent, handle, nullptr); }
template<> ApiHandle<VkRenderPass, VkDevice>::~ApiHandle() { if(handle) VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyRenderPass(parent, handle, nullptr); }
template<> ApiHandle<VkCommandPool, VkDevice>::~ApiHandle() { if(handle) VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyCommandPool(parent, handle, nullptr); }
template<> ApiHandle<VkCommandBuffer, HandlePair<VkDevice,VkCommandPool>>::~ApiHandle() { if(handle) VULKAN_HPP_DEFAULT_DISPATCHER.vkFreeCommandBuffers(parent.first, parent.second, 1, &handle); }
template<> ApiHandle<VkMemory, VkDevice>::~ApiHandle() { }
template<> ApiHandle<VkBuffer, VkDevice>::~ApiHandle() { if(handle) VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyBuffer(parent, handle, nullptr); }
template<> ApiHandle<VkBufferView, VkDevice>::~ApiHandle() { if(handle) VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyBufferView(parent, handle, nullptr); }
template<> ApiHandle<VkPipeline, VkDevice>::~ApiHandle() { if(handle) VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyPipeline(parent, handle, nullptr); }
template<> ApiHandle<VkPipelineCache, VkDevice>::~ApiHandle() { if(handle) VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyPipelineCache(parent, handle, nullptr); }
template<> ApiHandle<VkPipelineLayout, VkDevice>::~ApiHandle() { if(handle) VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyPipelineLayout(parent, handle, nullptr); }
template<> ApiHandle<VkShaderModule, VkDevice>::~ApiHandle() { if(handle) VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyShaderModule(parent, handle, nullptr); }
template<> ApiHandle<VkDescriptorSetLayout, VkDevice>::~ApiHandle() { if(handle) VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyDescriptorSetLayout(parent, handle, nullptr); }
template<> ApiHandle<VkDescriptorPool, VkDevice>::~ApiHandle() { if(handle) VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyDescriptorPool(parent, handle, nullptr); }
template<> ApiHandle<VkSampler, VkDevice>::~ApiHandle() { if(handle) VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroySampler(parent, handle, nullptr); }
template<> ApiHandle<VkDescriptorSet, HandlePair<VkDevice,VkDescriptorPool>>::~ApiHandle() { if(handle) VULKAN_HPP_DEFAULT_DISPATCHER.vkFreeDescriptorSets(parent.first, parent.second, 1, &handle); }
template<> ApiHandle<VmaAllocator, VkDevice>::~ApiHandle() { if(handle) vmaDestroyAllocator(handle); }
template<> ApiHandle<VmaAllocation, VmaAllocator>::~ApiHandle() { if(handle) vmaFreeMemory(parent, handle); }

} /* Rendering */
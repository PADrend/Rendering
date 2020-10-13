/*
	This file is part of the Rendering library.
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_VK_HANDLES
#define RENDERING_VK_HANDLES

#include <Util/ReferenceCounter.h>

#include <memory>

/** @defgroup rendering_core Core
 * Provides a low-level API to the rendering backend (Vulkan).
 */

/** @addtogroup rendering_core
 * @{
 */

//! @defgroup rendering_handles Handles

//! @}

#define API_HANDLE_DECLARE(Handle) typedef struct Vk##Handle##_T* Vk##Handle
#define API_HANDLE_DECLARE_MA(Handle) typedef struct Vma##Handle##_T* Vma##Handle

#define API_BASE_HANDLE(HandleType, ParentType, ApiType, ParentApiType) class HandleType##Handle { \
public: \
	using Ptr = std::unique_ptr<HandleType##Handle>; \
	HandleType##Handle() {} \
	HandleType##Handle(const ApiType& handle, const ParentApiType& parent = nullptr, bool owner = true) : parent(parent), handle(handle), owner(owner) {} \
	HandleType##Handle(const HandleType##Handle&) = delete; \
	HandleType##Handle(HandleType##Handle&& rhs) { parent = std::move(rhs.parent); handle = std::move(rhs.handle); owner = std::move(rhs.owner); rhs.parent = nullptr; rhs.handle = nullptr; } \
	~HandleType##Handle(); \
	HandleType##Handle& operator =(const HandleType##Handle&) = delete; \
	HandleType##Handle& operator =(HandleType##Handle&& rhs) { parent = std::move(rhs.parent); handle = std::move(rhs.handle); owner = std::move(rhs.owner); rhs.parent = nullptr; rhs.handle = nullptr; return *this; } \
	operator bool() const { return handle; } \
	operator const ParentApiType&() const { return parent; } \
	operator const ApiType&() const { return handle; } \
private: \
	ParentApiType parent = nullptr; \
	ApiType handle = nullptr; \
	bool owner = true; \
}

#define API_HANDLE(HandleType, ParentType) API_BASE_HANDLE(HandleType, ParentType, Vk##HandleType, Vk##ParentType)
#define API_HANDLE_KHR(HandleType, ParentType) API_BASE_HANDLE(HandleType, ParentType, Vk##HandleType##KHR, Vk##ParentType)
#define API_HANDLE_MA(HandleType, ParentType) API_BASE_HANDLE(HandleType, ParentType, Vma##HandleType, Vk##ParentType)
#define API_HANDLE_MA2(HandleType, ParentType) API_BASE_HANDLE(HandleType, ParentType, Vma##HandleType, Vma##ParentType)

typedef void* VkNullHandle;
API_HANDLE_DECLARE(Instance);
API_HANDLE_DECLARE(Device);
API_HANDLE_DECLARE(PhysicalDevice);
API_HANDLE_DECLARE(SurfaceKHR);
API_HANDLE_DECLARE(SwapchainKHR);
API_HANDLE_DECLARE(Queue);
API_HANDLE_DECLARE(CommandBuffer);
API_HANDLE_DECLARE(CommandPool);
API_HANDLE_DECLARE(Fence);
API_HANDLE_DECLARE(Memory);
API_HANDLE_DECLARE(Image);
API_HANDLE_DECLARE(ImageView);
API_HANDLE_DECLARE(Buffer);
API_HANDLE_DECLARE(BufferView);
API_HANDLE_DECLARE(Framebuffer);
API_HANDLE_DECLARE(RenderPass);
API_HANDLE_DECLARE(Pipeline);
API_HANDLE_DECLARE(ShaderModule);
API_HANDLE_DECLARE_MA(Allocator);
API_HANDLE_DECLARE_MA(Allocation);

namespace Rendering {
/** @addtogroup rendering_handles
 * @{
 */

API_HANDLE(Instance, NullHandle);
API_HANDLE(Device, PhysicalDevice);
API_HANDLE_KHR(Surface, Instance);
API_HANDLE_KHR(Swapchain, Device);
API_HANDLE(Queue, Device);
API_HANDLE(Fence, Device);
API_HANDLE(Memory, Device);
API_HANDLE(Image, Device);
API_HANDLE(ImageView, Device);
API_HANDLE(Buffer, Device);
API_HANDLE(BufferView, Device);
API_HANDLE(CommandBuffer, Device);
API_HANDLE(CommandPool, Device);
API_HANDLE(Framebuffer, Device);
API_HANDLE(RenderPass, Device);
API_HANDLE(Pipeline, Device);
API_HANDLE(ShaderModule, Device);
API_HANDLE_MA(Allocator, Device);
API_HANDLE_MA2(Allocation, Allocator);

//! @}

} /* Rendering */

#endif /* end of include guard: RENDERING_VK_HANDLES */

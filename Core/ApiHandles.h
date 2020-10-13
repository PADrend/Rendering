/*
	This file is part of the Rendering library.
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_VK_HANDLES
#define RENDERING_VK_HANDLES

#include <utility>

#include <Util/ReferenceCounter.h>

/** @defgroup rendering_core Core
 * Provides a low-level API to the rendering backend (Vulkan).
 */

/** @addtogroup rendering_core
 * @{
 */

//! @defgroup rendering_handles Handles

//! @}

template<typename TypeA, typename TypeB>
struct HandlePair {
	HandlePair() {}
	HandlePair(TypeA a, TypeB b = nullptr) : first(a), second(b) {}
	TypeA first = nullptr;
	TypeB second = nullptr;
};

//----------------

#define API_HANDLE_DECLARE(Handle) typedef struct Vk##Handle##_T* Vk##Handle
#define API_HANDLE_DECLARE_MA(Handle) typedef struct Vma##Handle##_T* Vma##Handle

#define API_BASE_HANDLE(HandleType, ApiType, ParentApiType) template<> ApiHandle<ApiType, ParentApiType>::~ApiHandle(); \
	using HandleType##Handle = ApiHandle<ApiType, ParentApiType>::Ref;

#define API_HANDLE(HandleType, ParentType) API_BASE_HANDLE(HandleType, Vk##HandleType, Vk##ParentType)
#define API_HANDLE_KHR(HandleType, ParentType) API_BASE_HANDLE(HandleType, Vk##HandleType##KHR, Vk##ParentType)
#define API_HANDLE_MA(HandleType, ParentType) API_BASE_HANDLE(HandleType, Vma##HandleType, Vk##ParentType)
#define API_HANDLE_MA2(HandleType, ParentType) API_BASE_HANDLE(HandleType, Vma##HandleType, Vma##ParentType)
#define API_HANDLE_DUAL_PARENT(HandleType, ParentTypeA, ParentTypeB) typedef HandlePair<Vk##ParentTypeA,Vk##ParentTypeB> HandleType##Parent; API_BASE_HANDLE(HandleType, Vk##HandleType, HandleType##Parent)

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
API_HANDLE_DECLARE(PipelineCache);
API_HANDLE_DECLARE(PipelineLayout);
API_HANDLE_DECLARE(ShaderModule);
API_HANDLE_DECLARE(DescriptorSet);
API_HANDLE_DECLARE(DescriptorSetLayout);
API_HANDLE_DECLARE(DescriptorPool);
API_HANDLE_DECLARE(Sampler);
API_HANDLE_DECLARE_MA(Allocator);
API_HANDLE_DECLARE_MA(Allocation);

namespace Rendering {
/** @addtogroup rendering_handles
 * @{
 */

//----------------

class ApiBaseHandle : public Util::ReferenceCounter<ApiBaseHandle> {
public:
	using Ref = Util::Reference<ApiBaseHandle>;
	virtual ~ApiBaseHandle() = default;
};

//----------------

template<class ApiType, class ParentApiType>
class ApiHandle : public ApiBaseHandle {
public:
	class Ref : public Util::Reference<ApiHandle<ApiType,ParentApiType>> {
	public:
		using BaseType_t = ApiHandle<ApiType,ParentApiType>;
		Ref() = default;
		Ref(BaseType_t* handle) : Util::Reference<BaseType_t>(handle) {}
		Ref(const ApiBaseHandle::Ref& base) : Ref(dynamic_cast<BaseType_t*>(base.get())) {}
		static Ref create(const ApiType& handle=nullptr, const ParentApiType& parent = nullptr) { return new BaseType_t(handle, parent); }
		operator const ApiType() const { return this->get() ? this->get()->handle : nullptr; }
		operator const ParentApiType() const { return this->get() ? this->get()->parent : nullptr; }
	};
	~ApiHandle() {};
	ApiHandle(ApiHandle&& rhs) { parent = std::move(rhs.parent); handle = std::move(rhs.handle); rhs.parent = nullptr; rhs.handle = nullptr; };
	ApiHandle(const ApiHandle& rhs) = delete;
	ApiHandle& operator=(ApiHandle&& rhs) = delete;
	ApiHandle& operator=(const ApiHandle& rhs) = delete;

	operator bool() const { return handle; }
	operator const ParentApiType&() const { return parent; }
	operator const ApiType&() const { return handle; }
private:
	ApiHandle() = default;
	ApiHandle(const ApiType& handle, const ParentApiType& parent = nullptr) : parent(parent), handle(handle) {}
	ApiType handle = nullptr;
	ParentApiType parent = nullptr;
};

//----------------

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
API_HANDLE_DUAL_PARENT(CommandBuffer, Device, CommandPool);
API_HANDLE(CommandPool, Device);
API_HANDLE(Framebuffer, Device);
API_HANDLE(RenderPass, Device);
API_HANDLE(Pipeline, Device);
API_HANDLE(PipelineCache, Device);
API_HANDLE(PipelineLayout, Device);
API_HANDLE(ShaderModule, Device);
API_HANDLE(DescriptorSetLayout, Device);
API_HANDLE(DescriptorPool, Device);
API_HANDLE(Sampler, Device);
API_HANDLE_DUAL_PARENT(DescriptorSet, Device, DescriptorPool);
API_HANDLE_MA(Allocator, Device);
API_HANDLE_MA2(Allocation, Allocator);

//! @}

} /* Rendering */

#endif /* end of include guard: RENDERING_VK_HANDLES */

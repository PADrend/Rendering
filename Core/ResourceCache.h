/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef RENDERING_CORE_RESOURCECACHE_H_
#define RENDERING_CORE_RESOURCECACHE_H_

#include "Common.h"
#include "../State/PipelineState.h"
#include "../State/ShaderLayout.h"

#include <Util/StringIdentifier.h>
#include <Util/Factory/ObjectCache.h>

namespace Rendering {
class Device;
using DeviceRef = Util::Reference<Device>;
class Shader;
using ShaderRef = Util::Reference<Shader>;
class FBO;
using FBORef = Util::Reference<FBO>;

class ResourceCache : public Util::ReferenceCounter<ResourceCache> {
public:
	using Ref = Util::Reference<ResourceCache>;
	RENDERINGAPI static Ref create(const DeviceRef& device);
	RENDERINGAPI ~ResourceCache();
	ResourceCache(ResourceCache&& o) = default;
	ResourceCache(const ResourceCache& o) = delete;
	ResourceCache& operator=(ResourceCache&& o) = default;
	ResourceCache& operator=(const ResourceCache& o) = default;

	RENDERINGAPI PipelineHandle createPipeline(const PipelineState& state, const PipelineHandle& parent);
	RENDERINGAPI DescriptorSetLayoutHandle createDescriptorSetLayout(const ShaderResourceLayoutSet& layout);
	RENDERINGAPI PipelineLayoutHandle createPipelineLayout(const ShaderLayout& layout);
	RENDERINGAPI RenderPassHandle createRenderPass(const FramebufferFormat& attachments);
	RENDERINGAPI RenderPassHandle createRenderPass(const FBORef& fbo, const std::vector<ResourceUsage>& lastColorUsages, ResourceUsage lastDepthUsage, bool clearColor=false, bool clearDepth=false, bool clearStencil=false);
	RENDERINGAPI FramebufferHandle createFramebuffer(const FBORef& fbo, const RenderPassHandle& renderPass);
private:
	ResourceCache(const DeviceRef& device);
	Util::WeakPointer<Device> device;

	Util::ObjectCache<ApiBaseHandle::Ref, Util::StringIdentifier> cache;

	template<class HandleType, typename ...Args>
	HandleType create(const Util::StringIdentifier& id, Args... args) {
		return HandleType(cache.create<Device*,Args...>(id, device.get(), args...));
	}
};

} /* Rendering */

#endif /* end of include guard: RENDERING_CORE_RESOURCECACHE_H_ */
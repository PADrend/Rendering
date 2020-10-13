/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "ResourceCache.h"
#include "../FBO.h"
#include "../Shader/Shader.h"
#include "../Texture/Texture.h"

namespace Rendering {

static const Util::StringIdentifier PIPELINE("Pipeline");
static const Util::StringIdentifier DESCRIPTORSET_LAYOUT("DescriptorSetLayout");
static const Util::StringIdentifier PIPELINE_LAYOUT("PipelineLayout");
static const Util::StringIdentifier RENDERPASS("RenderPass");
static const Util::StringIdentifier FRAMEBUFFER("Framebuffer");

//----------------

// defined in internal/VkPipeline.cpp
ApiBaseHandle::Ref createPipelineHandle(Device*, const PipelineState&, VkPipeline);
ApiBaseHandle::Ref createPipelineLayoutHandle(Device*, const ShaderLayout&);
ApiBaseHandle::Ref createDescriptorSetLayoutHandle(Device*, const ShaderResourceLayoutSet&);

// defined in internal/VkFramebuffer.cpp
ApiBaseHandle::Ref createRenderPassHandle(Device*, const FramebufferFormat&, bool, bool, bool, const std::vector<ResourceUsage>&, ResourceUsage);
ApiBaseHandle::Ref createFramebufferHandle(Device*, FBO*, VkRenderPass);

//----------------

ResourceCache::Ref ResourceCache::create(const DeviceRef& device) {
	return new ResourceCache(device);
}

//----------------

ResourceCache::ResourceCache(const DeviceRef& device) : device(device.get()) {
	cache.registerType(PIPELINE, std::function<decltype(createPipelineHandle)>(createPipelineHandle));
	cache.registerType(DESCRIPTORSET_LAYOUT, std::function<decltype(createDescriptorSetLayoutHandle)>(createDescriptorSetLayoutHandle));
	cache.registerType(PIPELINE_LAYOUT, std::function<decltype(createPipelineLayoutHandle)>(createPipelineLayoutHandle));
	cache.registerType(RENDERPASS, std::function<decltype(createRenderPassHandle)>(createRenderPassHandle));
	cache.registerType(FRAMEBUFFER, std::function<decltype(createFramebufferHandle)>(createFramebufferHandle));
}


//----------------

PipelineHandle ResourceCache::createPipeline(const PipelineState& state, const PipelineHandle& parent) {
	return create<PipelineHandle, const PipelineState&, VkPipeline>(PIPELINE, state, parent);
}

//----------------

DescriptorSetLayoutHandle ResourceCache::createDescriptorSetLayout(const ShaderResourceLayoutSet& layout) {
	return create<DescriptorSetLayoutHandle, const ShaderResourceLayoutSet&>(DESCRIPTORSET_LAYOUT, layout);
}

//----------------

PipelineLayoutHandle ResourceCache::createPipelineLayout(const ShaderLayout& layout) {
	return create<PipelineLayoutHandle, const ShaderLayout&>(PIPELINE_LAYOUT, layout);
}

//----------------

RenderPassHandle ResourceCache::createRenderPass(const FramebufferFormat& attachments) {
	return create<RenderPassHandle, const FramebufferFormat&, bool, bool, bool, const std::vector<ResourceUsage>&, ResourceUsage>(RENDERPASS, attachments, false, false, false, {}, {});
}

//----------------

RenderPassHandle ResourceCache::createRenderPass(const FBORef& fbo, bool clearColor, bool clearDepth, bool clearStencil) {
	if(!fbo)
		return nullptr;
	std::vector<ResourceUsage> lastColorUsages;
	for(auto& att : fbo->getColorAttachments())
		lastColorUsages.emplace_back(att.isNotNull() ? att->getLastUsage() : ResourceUsage::Undefined);
	ResourceUsage lastDepthUsage = fbo->getDepthStencilAttachment() ? fbo->getDepthStencilAttachment()->getLastUsage() : ResourceUsage::Undefined;
	return create<RenderPassHandle, const FramebufferFormat&, bool, bool, bool, const std::vector<ResourceUsage>&, ResourceUsage>(RENDERPASS, {fbo}, clearColor, clearDepth, clearStencil, lastColorUsages, lastDepthUsage);
}

//----------------

FramebufferHandle ResourceCache::createFramebuffer(const FBORef& fbo, const RenderPassHandle& renderPass) {
	return create<FramebufferHandle, FBO*, VkRenderPass>(FRAMEBUFFER, fbo.get(), renderPass);
}

//----------------
} /* Rendering */
//---------------------------

template <> struct std::hash<std::vector<Rendering::ResourceUsage>> {
	std::size_t operator()(const std::vector<Rendering::ResourceUsage>& values) const {
		std::size_t result = 0;
		Util::hash_combine(result, values.size());
		for(auto& v : values)
			Util::hash_combine(result, v);
		return result;
	}
};
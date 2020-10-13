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

namespace Rendering {

static const Util::StringIdentifier COMPUTE_PIPELINE("ComputePipeline");
static const Util::StringIdentifier GRAPHICS_PIPELINE("GraphicsPipeline");
static const Util::StringIdentifier DESCRIPTORSET_LAYOUT("DescriptorSetLayout");
static const Util::StringIdentifier PIPELINE_LAYOUT("PipelineLayout");

//----------------

// defined in Pipeline.cpp
ApiBaseHandle::Ref createComputePipelineHandle(Device* device, Shader* shader, const std::string& entryPoint, VkPipeline parent);
ApiBaseHandle::Ref createGraphicsPipelineHandle(Device* device, Shader* shader, const PipelineState& state, VkPipeline parent);
ApiBaseHandle::Ref createPipelineLayoutHandle(Device* device, const ShaderLayout& layout);
// defined in DescriptorSet.cpp
ApiBaseHandle::Ref createDescriptorSetLayoutHandle(Device* device, const ShaderResourceLayoutSet& layoutSet);

//----------------

ResourceCache::Ref ResourceCache::create(const DeviceRef& device) {
	return new ResourceCache(device);
}

//----------------

ResourceCache::ResourceCache(const DeviceRef& device) : device(device) {
	cache.registerType(COMPUTE_PIPELINE, std::function<decltype(createComputePipelineHandle)>(createComputePipelineHandle));
	cache.registerType(GRAPHICS_PIPELINE, std::function<decltype(createGraphicsPipelineHandle)>(createGraphicsPipelineHandle));
	cache.registerType(DESCRIPTORSET_LAYOUT, std::function<decltype(createDescriptorSetLayoutHandle)>(createDescriptorSetLayoutHandle));
	cache.registerType(PIPELINE_LAYOUT, std::function<decltype(createPipelineLayoutHandle)>(createPipelineLayoutHandle));
}

//----------------

PipelineHandle ResourceCache::createComputePipeline(const ShaderRef& shader, const std::string& entryPoint, const PipelineHandle& parent) {
	return create<PipelineHandle, Shader*, const std::string&, VkPipeline>(COMPUTE_PIPELINE, shader.get(), entryPoint, parent);
}

//----------------

PipelineHandle ResourceCache::createGraphicsPipeline(const ShaderRef& shader, const PipelineState& state, const PipelineHandle& parent) {
	return create<PipelineHandle, Shader*, const PipelineState&, VkPipeline>(GRAPHICS_PIPELINE, shader, state, parent);
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
} /* Rendering */

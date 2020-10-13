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

ApiBaseHandle::Ref createComputePipelineHandle(Device* device, const PipelineState& state, VkPipeline parent);
ApiBaseHandle::Ref createGraphicsPipelineHandle(Device* device, const PipelineState& state, VkPipeline parent);

//----------------

ResourceCache::Ref ResourceCache::create(const DeviceRef& device) {
	return new ResourceCache(device);
}

//----------------

ResourceCache::ResourceCache(const DeviceRef& device) : device(device) {
	cache.registerType(COMPUTE_PIPELINE, std::function<decltype(createComputePipelineHandle)>(createComputePipelineHandle));
	cache.registerType(GRAPHICS_PIPELINE, std::function<decltype(createGraphicsPipelineHandle)>(createGraphicsPipelineHandle));
}

//----------------

PipelineHandle ResourceCache::createComputePipeline(const PipelineState& state, const PipelineHandle& parent) {
	return create<PipelineHandle, const PipelineState&, VkPipeline>(COMPUTE_PIPELINE, state, parent);
	return nullptr;
}

//----------------

PipelineHandle ResourceCache::createGraphicsPipeline(const PipelineState& state, const PipelineHandle& parent) {
	return create<PipelineHandle, const PipelineState&, VkPipeline>(GRAPHICS_PIPELINE, state, parent);
}

//----------------
} /* Rendering */

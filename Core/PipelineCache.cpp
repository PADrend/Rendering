/*
	This file is part of the Rendering library.
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "PipelineCache.h"
#include "Device.h"
#include "../Shader/Shader.h"
#include "../FBO.h"

#include <Util/Utils.h>
#include <Util/Macros.h>

#include <vulkan/vulkan.hpp>

#include <vector>

namespace Rendering {


PipelineCache::~PipelineCache() = default;

//---------------

PipelineCache::PipelineCache(const DeviceRef& device) : device(device) { }

//---------------

bool PipelineCache::init() {
	vk::Device vkDevice(device->getApiHandle());
	handle = PipelineCacheHandle::create(vkDevice.createPipelineCache({}), vkDevice);
	return true;
}

//---------------

Pipeline::Ref PipelineCache::requestPipeline(PipelineType type, const PipelineState& state, const Pipeline::Ref& parent) {		
	size_t hash = 0;
	if(type == PipelineType::Graphics) {
		Util::hash_combine(hash, state);
	} else {
		auto& shader = state.getShader();
		Util::hash_combine(hash, shader ? shader->getLayoutHash() : 0);
		Util::hash_combine(hash, state.getEntryPoint());
	}

	// Search for existing pipelines
	auto it = cache.find(hash);
	if(it != cache.end())
		return it->second;

	// Create new pipeline
	Pipeline::Ref pipeline = new Pipeline(type, state, parent);
	if(!pipeline->init(this))
		return nullptr;

	cache.emplace(hash, pipeline);

	return pipeline;
}

//---------------

Pipeline::Ref PipelineCache::requestGraphicsPipeline(const PipelineState& state, const Pipeline::Ref& parent) {
	return requestPipeline(PipelineType::Graphics, state, parent);
}

//---------------

Pipeline::Ref PipelineCache::requestComputePipeline(const ShaderRef& shader, const std::string& entrypoint, const Pipeline::Ref& parent) {
	PipelineState state{};
	state.setShader(shader);
	state.setEntryPoint(entrypoint);
	return requestPipeline(PipelineType::Compute, state, parent);
}

//---------------

} /* Rendering */
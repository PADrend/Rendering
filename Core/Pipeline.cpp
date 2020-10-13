/*
	This file is part of the Rendering library.
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "Pipeline.h"
#include "Device.h"
#include "PipelineCache.h"
#include "../FBO.h"
#include "../Shader/Shader.h"

#include <Util/Macros.h>

namespace Rendering {

//---------------

Pipeline::~Pipeline() = default;

//---------------

Pipeline::Pipeline(const DeviceRef& device, PipelineType type) : type(type), device(device) { }

//---------------

bool Pipeline::validate() {
	switch(type) {
		case PipelineType::Graphics:
			handle = device->getPipelineCache()->requestGraphicsHandle(this, nullptr);
			break;
		case PipelineType::Compute:
			handle = device->getPipelineCache()->requestComputeHandle(this, nullptr);
			break;
		default:
			handle = nullptr;
	}
	return handle;
}

//---------------

void Pipeline::setShader(const ShaderRef& value) { invalidate(); shader = value; }

//---------------

void Pipeline::reset() { 
	invalidate();
	shader = nullptr;
}

//---------------

void GraphicsPipeline::setState(const PipelineState& value) { invalidate(); state = value; }

//---------------

void GraphicsPipeline::setFBO(const FBORef& value) { invalidate(); fbo = value; }

//---------------

void GraphicsPipeline::reset() {
	Pipeline::reset();
	fbo = nullptr;
	state = {};
}

//---------------

void ComputePipeline::setEntryPoint(const std::string& value) { invalidate(); entryPoint = value; }

//---------------

void ComputePipeline::reset() {
	Pipeline::reset();
	entryPoint = "main";
}

//---------------

GraphicsPipeline::Ref GraphicsPipeline::create(const DeviceRef& device, const PipelineState& state, const ShaderRef& shader, const FBORef& fbo) {
	auto pipeline = new GraphicsPipeline(device);
	pipeline->setState(state);
	pipeline->setShader(shader);
	pipeline->setFBO(fbo);
	return pipeline;
}

//---------------

ComputePipeline::Ref ComputePipeline::create(const DeviceRef& device, const ShaderRef& shader, const std::string& entryPoint) {
	auto pipeline = new ComputePipeline(device);
	pipeline->setShader(shader);
	pipeline->setEntryPoint(entryPoint);
	return pipeline;
}

//---------------

} /* Rendering */
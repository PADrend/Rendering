/*
	This file is part of the Platform for Algorithm Development and Rendering (PADrend).
	Web page: http://www.padrend.de/
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	Copyright (C) 2014-2022 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "PipelineBuilder.h"
#include "RenderDevice.h"
#include <Util/StringUtils.h>

#include <stack>
#include <unordered_map>

namespace Rendering {
using namespace Util;

using ShaderStack = std::stack<nvrhi::ShaderHandle>;
using BindingLayoutStack = std::stack<nvrhi::BindingLayoutHandle>;

struct PipelineBuilder::Internal {
	Internal(const RenderDeviceHandle& device) : device(device) {}

	RenderDeviceHandle device;

	// TODO: pipeline cache?

	std::unordered_map<nvrhi::ShaderType, ShaderStack> shaderStacks;
	std::unordered_map<nvrhi::ShaderType, nvrhi::ShaderHandle> shaders;

	std::array<BindingLayoutStack, nvrhi::c_MaxBindingLayouts> bindingLayoutsStack;
	nvrhi::BindingLayoutVector bindingLayouts;

	std::stack<nvrhi::FramebufferHandle> framebufferStack;
	nvrhi::FramebufferHandle framebuffer;

	std::stack<nvrhi::InputLayoutHandle> inputLayoutStack;
	nvrhi::InputLayoutHandle inputLayout;

	std::stack<nvrhi::PrimitiveType> primitiveTypeStack;
	nvrhi::PrimitiveType primitiveType = nvrhi::PrimitiveType::TriangleList;

	std::stack<uint32_t> patchControlPointsStack;
	uint32_t patchControlPoints = 0;

	std::stack<nvrhi::BlendState> blendStateStack;
	std::stack<nvrhi::RasterState> rasterStateStack;
	std::stack<nvrhi::DepthStencilState> depthStencilStateStack;
	nvrhi::RenderState renderState;
	
	std::stack<nvrhi::VariableRateShadingState> variableRateShadingStateStack;
	nvrhi::VariableRateShadingState variableRateShadingState;
};

//----------------------

PipelineBuilder::PipelineBuilder(const RenderDeviceHandle& device) : data(std::make_unique<Internal>(device)) {
}

//----------------------

PipelineBuilder::~PipelineBuilder() = default;

//----------------------

PipelineBuilder::PipelineBuilder(PipelineBuilder&& o) {
	data.swap(o.data);
}

//----------------------

PipelineBuilder& PipelineBuilder::operator=(PipelineBuilder&& o) {
	data.swap(o.data);
	return *this;
}

//----------------------

nvrhi::GraphicsPipelineHandle PipelineBuilder::createGraphicsPipeline() const {
	auto nvDevice = data->device->_getInternalDevice();
	auto desc = nvrhi::GraphicsPipelineDesc()
		.setPrimType(data->primitiveType)
		.setPatchControlPoints(data->patchControlPoints)
		.setVertexShader(data->shaders[nvrhi::ShaderType::Vertex])
		.setHullShader(data->shaders[nvrhi::ShaderType::Hull])
		.setDomainShader(data->shaders[nvrhi::ShaderType::Domain])
		.setGeometryShader(data->shaders[nvrhi::ShaderType::Geometry])
		.setPixelShader(data->shaders[nvrhi::ShaderType::Pixel])
		.setRenderState(data->renderState)
		.setVariableRateShadingState(data->variableRateShadingState);
	desc.bindingLayouts = data->bindingLayouts;

	// TODO: validate state

	return nvDevice->createGraphicsPipeline(desc, data->framebuffer);
}

//----------------------

nvrhi::ComputePipelineHandle PipelineBuilder::createComputePipeline() const {
	auto nvDevice = data->device->_getInternalDevice();
	auto desc = nvrhi::ComputePipelineDesc()
		.setComputeShader(data->shaders[nvrhi::ShaderType::Compute]);
	desc.bindingLayouts = data->bindingLayouts;

	// TODO: validate state

	return nvDevice->createComputePipeline(desc);
}

//----------------------

nvrhi::MeshletPipelineHandle PipelineBuilder::createMeshletPipeline() const {
	auto nvDevice = data->device->_getInternalDevice();
	auto desc = nvrhi::MeshletPipelineDesc()
		.setPrimType(data->primitiveType)
		.setAmplificationShader(data->shaders[nvrhi::ShaderType::Amplification])
		.setMeshShader(data->shaders[nvrhi::ShaderType::Mesh])
		.setPixelShader(data->shaders[nvrhi::ShaderType::Pixel])
		.setRenderState(data->renderState);
	desc.bindingLayouts = data->bindingLayouts;

	// TODO: validate state

	return nvDevice->createMeshletPipeline(desc, data->framebuffer);
}

//----------------------

void PipelineBuilder::setShader(const nvrhi::ShaderHandle& shader, nvrhi::ShaderType type) {
	switch(type) {
		case nvrhi::ShaderType::Compute:
		case nvrhi::ShaderType::Vertex:
		case nvrhi::ShaderType::Hull:
		case nvrhi::ShaderType::Domain:
		case nvrhi::ShaderType::Geometry:
		case nvrhi::ShaderType::Pixel:
		case nvrhi::ShaderType::Amplification:
		case nvrhi::ShaderType::Mesh:
		case nvrhi::ShaderType::RayGeneration:
		case nvrhi::ShaderType::AnyHit:
		case nvrhi::ShaderType::ClosestHit:
		case nvrhi::ShaderType::Miss:
		case nvrhi::ShaderType::Intersection:
		case nvrhi::ShaderType::Callable:
			data->shaders[type] = shader;
			break;
		default:
			break;
	}
}

//----------------------

nvrhi::ShaderHandle PipelineBuilder::getShader(nvrhi::ShaderType type) const {
	switch(type) {
		case nvrhi::ShaderType::Compute:
		case nvrhi::ShaderType::Vertex:
		case nvrhi::ShaderType::Hull:
		case nvrhi::ShaderType::Domain:
		case nvrhi::ShaderType::Geometry:
		case nvrhi::ShaderType::Pixel:
		case nvrhi::ShaderType::Amplification:
		case nvrhi::ShaderType::Mesh:
		case nvrhi::ShaderType::RayGeneration:
		case nvrhi::ShaderType::AnyHit:
		case nvrhi::ShaderType::ClosestHit:
		case nvrhi::ShaderType::Miss:
		case nvrhi::ShaderType::Intersection:
		case nvrhi::ShaderType::Callable:
			return data->shaders[type];
		default:
			return nullptr;
	}
}

//----------------------

void PipelineBuilder::pushShader(nvrhi::ShaderType type) {
	switch(type) {
		case nvrhi::ShaderType::Compute:
		case nvrhi::ShaderType::Vertex:
		case nvrhi::ShaderType::Hull:
		case nvrhi::ShaderType::Domain:
		case nvrhi::ShaderType::Geometry:
		case nvrhi::ShaderType::Pixel:
		case nvrhi::ShaderType::Amplification:
		case nvrhi::ShaderType::Mesh:
		case nvrhi::ShaderType::RayGeneration:
		case nvrhi::ShaderType::AnyHit:
		case nvrhi::ShaderType::ClosestHit:
		case nvrhi::ShaderType::Miss:
		case nvrhi::ShaderType::Intersection:
		case nvrhi::ShaderType::Callable:
			data->shaderStacks[type].emplace(data->shaders[type]);
			break;
		default:
			break;
	}
}

//----------------------

void PipelineBuilder::popShader(nvrhi::ShaderType type) {
	switch(type) {
		case nvrhi::ShaderType::Compute:
		case nvrhi::ShaderType::Vertex:
		case nvrhi::ShaderType::Hull:
		case nvrhi::ShaderType::Domain:
		case nvrhi::ShaderType::Geometry:
		case nvrhi::ShaderType::Pixel:
		case nvrhi::ShaderType::Amplification:
		case nvrhi::ShaderType::Mesh:
		case nvrhi::ShaderType::RayGeneration:
		case nvrhi::ShaderType::AnyHit:
		case nvrhi::ShaderType::ClosestHit:
		case nvrhi::ShaderType::Miss:
		case nvrhi::ShaderType::Intersection:
		case nvrhi::ShaderType::Callable:
			if(!data->shaderStacks[type].empty()) {
				data->shaders[type] = data->shaderStacks[type].top();
				data->shaderStacks[type].pop();
			}
			break;
		default:
			break;
	}
}

//----------------------

void PipelineBuilder::setBindingLayout(const nvrhi::BindingLayoutHandle& layout, uint32_t index) {
	WARN_AND_RETURN_IF(index >= nvrhi::c_MaxBindingLayouts, StringUtils::format("Invalid binding index %d", index),);
	data->bindingLayouts[index] = layout;
}

//----------------------

nvrhi::BindingLayoutHandle PipelineBuilder::getBindingLayout(uint32_t index) const {
	WARN_AND_RETURN_IF(index >= nvrhi::c_MaxBindingLayouts, StringUtils::format("Invalid binding index %d", index), nullptr);
	return data->bindingLayouts[index];
}

//----------------------

void PipelineBuilder::pushBindingLayout(uint32_t index) {
	WARN_AND_RETURN_IF(index >= nvrhi::c_MaxBindingLayouts, StringUtils::format("Invalid binding index %d", index),);
	data->bindingLayoutsStack[index].emplace(data->bindingLayouts[index]);
}

//----------------------

void PipelineBuilder::popBindingLayout(uint32_t index) {
	WARN_AND_RETURN_IF(index >= nvrhi::c_MaxBindingLayouts, StringUtils::format("Invalid binding index %d", index),);
	if(!data->bindingLayoutsStack[index].empty()) {
		data->bindingLayouts[index] = data->bindingLayoutsStack[index].top();
		data->bindingLayoutsStack[index].pop();
	}
}

//----------------------

void PipelineBuilder::setFramebuffer(const nvrhi::FramebufferHandle& framebuffer) {
	data->framebuffer = framebuffer;
}

//----------------------

nvrhi::FramebufferHandle PipelineBuilder::getFramebuffer() const {
	return data->framebuffer;
}

//----------------------

void PipelineBuilder::pushFramebuffer() {
	data->framebufferStack.emplace(data->framebuffer);
}

//----------------------

void PipelineBuilder::popFramebuffer() {
	if(!data->framebufferStack.empty()) {
		data->framebuffer = data->framebufferStack.top();
		data->framebufferStack.pop();
	}
}

//----------------------

void PipelineBuilder::setInputLayout(const nvrhi::InputLayoutHandle& layout) {
	data->inputLayout = layout;
}

//----------------------

nvrhi::InputLayoutHandle PipelineBuilder::getInputLayout() const {
	return data->inputLayout;
}

//----------------------

void PipelineBuilder::pushInputLayout() {
	data->inputLayoutStack.emplace(data->inputLayout);
}

//----------------------

void PipelineBuilder::popInputLayout() {
	if(!data->inputLayoutStack.empty()) {
		data->inputLayout = data->inputLayoutStack.top();
		data->inputLayoutStack.pop();
	}
}

//----------------------

void PipelineBuilder::setPrimitiveType(nvrhi::PrimitiveType type) {
	data->primitiveType = type;
}

//----------------------

nvrhi::PrimitiveType PipelineBuilder::getPrimitiveType() const {
	return data->primitiveType;
}

//----------------------

void PipelineBuilder::pushPrimitiveType() {
	data->primitiveTypeStack.emplace(data->primitiveType);
}

//----------------------

void PipelineBuilder::popPrimitiveType() {
	if(!data->primitiveTypeStack.empty()) {
		data->primitiveType = data->primitiveTypeStack.top();
		data->primitiveTypeStack.pop();
	}
}

//----------------------

void PipelineBuilder::setPatchControlPoints(uint32_t points) {
	data->patchControlPoints = points;
}

//----------------------

uint32_t PipelineBuilder::getPatchControlPoints() const {
	return data->patchControlPoints;
}

//----------------------

void PipelineBuilder::pushPatchControlPoints() {
	data->patchControlPointsStack.emplace(data->patchControlPoints);
}

//----------------------

void PipelineBuilder::popPatchControlPoints() {
	if(!data->patchControlPointsStack.empty()) {
		data->patchControlPoints = data->patchControlPointsStack.top();
		data->patchControlPointsStack.pop();
	}
}

//----------------------

const nvrhi::RenderState& PipelineBuilder::getRenderState() const {
	return data->renderState;
}

//----------------------

void PipelineBuilder::setBlendState(const nvrhi::BlendState& state) {
	data->renderState.blendState = state;
}

//----------------------

const nvrhi::BlendState& PipelineBuilder::getBlendState() const {
	return data->renderState.blendState;
}

//----------------------

void PipelineBuilder::pushBlendState() {
	data->blendStateStack.emplace(data->renderState.blendState);
}

//----------------------

void PipelineBuilder::popBlendState() {
	if(!data->blendStateStack.empty()) {
		data->renderState.blendState = data->blendStateStack.top();
		data->blendStateStack.pop();
	}
}

//----------------------

void PipelineBuilder::setRasterState(const nvrhi::RasterState& state) {
	data->renderState.rasterState = state;
}

//----------------------

const nvrhi::RasterState& PipelineBuilder::getRasterState() const {
	return data->renderState.rasterState;
}

//----------------------

void PipelineBuilder::pushRasterState() {
	data->rasterStateStack.emplace(data->renderState.rasterState);
}

//----------------------

void PipelineBuilder::popRasterState() {
	if(!data->rasterStateStack.empty()) {
		data->renderState.rasterState = data->rasterStateStack.top();
		data->rasterStateStack.pop();
	}
}

//----------------------

void PipelineBuilder::setDepthStencilState(const nvrhi::DepthStencilState& state) {
	data->renderState.depthStencilState = state;
}

//----------------------

const nvrhi::DepthStencilState& PipelineBuilder::getDepthStencilState() const {
	return data->renderState.depthStencilState;
}

//----------------------

void PipelineBuilder::pushDepthStencilState() {
	data->depthStencilStateStack.emplace(data->renderState.depthStencilState);
}

//----------------------

void PipelineBuilder::popDepthStencilState() {
	if(!data->depthStencilStateStack.empty()) {
		data->renderState.depthStencilState = data->depthStencilStateStack.top();
		data->depthStencilStateStack.pop();
	}
}

//----------------------

void PipelineBuilder::setVariableRateShadingState(const nvrhi::VariableRateShadingState& state) {
	data->variableRateShadingState = state;
}

//----------------------

const nvrhi::VariableRateShadingState& PipelineBuilder::getVariableRateShadingState() const {
	return data->variableRateShadingState;
}

//----------------------

void PipelineBuilder::pushVariableRateShadingState() {
	data->variableRateShadingStateStack.emplace(data->variableRateShadingState);
}

//----------------------

void PipelineBuilder::popVariableRateShadingState() {
	if(!data->variableRateShadingStateStack.empty()) {
		data->variableRateShadingState = data->variableRateShadingStateStack.top();
		data->variableRateShadingStateStack.pop();
	}
}

//----------------------


} // Rendering

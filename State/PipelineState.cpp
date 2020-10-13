/*
	This file is part of the Rendering library.
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "PipelineState.h"
#include "../Shader/Shader.h"
#include "../Texture/Texture.h"
#include "../FBO.h"

#include <Util/Macros.h>

namespace Rendering {

//-------------

ViewportState& ViewportState::setViewport(const Viewport& value, uint32_t index) {
	WARN_AND_RETURN_IF(viewports.size() <= index, "Invalid viewport index " + std::to_string(index), *this); 
	viewports[index] = value; 
	dirty = true;
	return *this; 
}

//-------------

ViewportState& ViewportState::setViewports(const std::vector<Viewport>& values) {
	setViewportScissorCount(values.size());
	viewports = values;
	dirty = true;
	return *this; 
}

//-------------

ViewportState& ViewportState::setScissor(const Geometry::Rect_i& value, uint32_t index) {
	WARN_AND_RETURN_IF(scissors.size() <= index, "Invalid scissor index " + std::to_string(index), *this); 
	dirty = true;
	scissors[index] = value; 
	return *this; 
}

//-------------

ViewportState& ViewportState::setScissors(const std::vector<Geometry::Rect_i>& values) {
	setViewportScissorCount(values.size());
	scissors = values;
	dirty = true;
	return *this; 
}

//-------------

ColorBlendState& ColorBlendState::setAttachment(const ColorBlendAttachmentState& value, uint32_t index) { 
	WARN_AND_RETURN_IF(attachments.size() <= index, "Invalid attachment index " + std::to_string(index), *this); 
	attachments[index] = value; 
	dirty = true;
	return *this; 
}

//---------------

FramebufferFormat::FramebufferFormat(const FBORef& fbo) {
	if(fbo) {
		colorAttachments.clear();
		for(const auto& att : fbo->getColorAttachments()) {
			if(att)
				colorAttachments.emplace_back(att->getFormat().pixelFormat, att->getFormat().samples);
			else
				colorAttachments.emplace_back(InternalFormat::Unknown, 0);
		}
		auto depthAtt = fbo->getDepthStencilAttachment();
		if(depthAtt)
			depthAttachment = {depthAtt->getFormat().pixelFormat, depthAtt->getFormat().samples};
		else
			depthAttachment = {InternalFormat::Unknown, 0};
	}
	dirty = true;
}

//---------------

PipelineState::PipelineState() { reset(); }

//-------------

PipelineState::PipelineState(PipelineState&& o) {
	setVertexInputState(std::move(o.vertexInput))
		.setInputAssemblyState(std::move(o.inputAssembly))
		.setViewportState(std::move(o.viewport))
		.setRasterizationState(std::move(o.rasterization))
		.setMultisampleState(std::move(o.multisample))
		.setDepthStencilState(std::move(o.depthStencil))
		.setColorBlendState(std::move(o.colorBlend))
		.setShader(std::move(o.shader))
		.setEntryPoint(std::move(o.entrypoint))
		.setFramebufferFormat(std::move(o.attachments));
	o.reset();
}

//-------------

PipelineState::PipelineState(const PipelineState& o) {
	setVertexInputState(o.vertexInput)
		.setInputAssemblyState(o.inputAssembly)
		.setViewportState(o.viewport)
		.setRasterizationState(o.rasterization)
		.setMultisampleState(o.multisample)
		.setDepthStencilState(o.depthStencil)
		.setColorBlendState(o.colorBlend)
		.setShader(o.shader)
		.setEntryPoint(o.entrypoint)
		.setFramebufferFormat(o.attachments);
}

//-------------

PipelineState& PipelineState::operator=(PipelineState&& o) {
	setVertexInputState(std::move(o.vertexInput))
		.setInputAssemblyState(std::move(o.inputAssembly))
		.setViewportState(std::move(o.viewport))
		.setRasterizationState(std::move(o.rasterization))
		.setMultisampleState(std::move(o.multisample))
		.setDepthStencilState(std::move(o.depthStencil))
		.setColorBlendState(std::move(o.colorBlend))
		.setShader(std::move(o.shader))
		.setEntryPoint(std::move(o.entrypoint))
		.setFramebufferFormat(std::move(o.attachments));
	o.reset();
	markAsChanged();
	return *this;
}

//-------------

PipelineState& PipelineState::operator=(const PipelineState& o) {
	setVertexInputState(o.vertexInput)
		.setInputAssemblyState(o.inputAssembly)
		.setViewportState(o.viewport)
		.setRasterizationState(o.rasterization)
		.setMultisampleState(o.multisample)
		.setDepthStencilState(o.depthStencil)
		.setColorBlendState(o.colorBlend)
		.setShader(o.shader)
		.setEntryPoint(o.entrypoint)
		.setFramebufferFormat(o.attachments);
	return *this;
}

//-------------

PipelineState& PipelineState::reset() {
	setVertexInputState({})
		.setInputAssemblyState({})
		.setViewportState({})
		.setRasterizationState({})
		.setMultisampleState({})
		.setDepthStencilState({})
		.setColorBlendState({})
		.setShader(nullptr)
		.setEntryPoint("main")
		.setFramebufferFormat(FramebufferFormat{});
	markAsChanged();
	return *this;
}

//-------------

PipelineState& PipelineState::setShader(const ShaderRef& _shader) {
	if(shader != _shader)
		markAsChanged();
	shader = _shader;
	return *this;
}

//-------------

void PipelineState::markAsChanged() {
	dirty = true;
	vertexInput.markAsChanged();
	inputAssembly.markAsChanged();
	viewport.markAsChanged();
	rasterization.markAsChanged();
	multisample.markAsChanged();
	depthStencil.markAsChanged();
	colorBlend.markAsChanged();
	attachments.markAsChanged();
}

//-------------

void PipelineState::markAsUnchanged() {
	vertexInput.markAsUnchanged();
	inputAssembly.markAsUnchanged();
	viewport.markAsUnchanged();
	rasterization.markAsUnchanged();
	multisample.markAsUnchanged();
	depthStencil.markAsUnchanged();
	colorBlend.markAsUnchanged();
	attachments.markAsUnchanged();
	dirty = false;
}

//-------------

bool PipelineState::hasChanged() const {
	if(!dirty)
		return false;
	
	return vertexInput.hasChanged() ||
		inputAssembly.hasChanged() ||
		viewport.hasChanged() ||
		rasterization.hasChanged() ||
		multisample.hasChanged() ||
		depthStencil.hasChanged() ||
		colorBlend.hasChanged() ||
		attachments.hasChanged();
}

//-------------

} /* Rendering */


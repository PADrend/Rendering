/*
	This file is part of the Rendering library.
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef RENDERING_CORE_PIPELINE_H_
#define RENDERING_CORE_PIPELINE_H_

#include "Common.h"
#include "../RenderingContext/PipelineState.h"

#include <Util/ReferenceCounter.h>

namespace Rendering {
class Device;
using DeviceRef = Util::Reference<Device>;
class Shader;
using ShaderRef = Util::Reference<Shader>;

//---------------

struct PipelineCreateInfo;

//---------------

class Pipeline : public Util::ReferenceCounter<Pipeline> {
public:
	using Ref = Util::Reference<Pipeline>;
	static Ref createCompute(const DeviceRef& device, const ShaderRef& shader=nullptr, const std::string& entryPoint="main", const Ref& parent=nullptr);
	static Ref createGraphics(const DeviceRef& device, const PipelineState& state={}, const Ref& parent=nullptr);

	Pipeline(Pipeline &&) = default;
	Pipeline(const Pipeline &) = delete;
	~Pipeline();

	bool validate();
	bool isValid() const;

	void setState(const PipelineState& value) { state = value; }
	const PipelineState& getState() const { return state; }
	PipelineState& getState() { return state; }
	
	void setVertexInputState(const VertexInputState& value) { state.setVertexInputState(value); }
	void setInputAssemblyState(const InputAssemblyState& value) { state.setInputAssemblyState(value); }
	void setViewportState(const ViewportState& value) { state.setViewportState(value); }
	void setRasterizationState(const RasterizationState& value) { state.setRasterizationState(value); }
	void setMultisampleState(const MultisampleState& value) { state.setMultisampleState(value); }
	void setDepthStencilState(const DepthStencilState& value) { state.setDepthStencilState(value); }
	void setColorBlendState(const ColorBlendState& value) { state.setColorBlendState(value); }
	void setEntryPoint(const std::string& value) { state.setEntryPoint(value); }
	void setShader(const ShaderRef& value);
	void setFBO(const FBORef& fbo) { state.setFBO(fbo); }
	
	const VertexInputState& getVertexInputState() const { return state.getVertexInputState(); }
	const InputAssemblyState& getInputAssemblyState() const { return state.getInputAssemblyState(); }
	const ViewportState& getViewportState() const { return state.getViewportState(); }
	const RasterizationState& getRasterizationState() const { return state.getRasterizationState(); }
	const MultisampleState& getMultisampleState() const { return state.getMultisampleState(); }
	const DepthStencilState& getDepthStencilState() const { return state.getDepthStencilState(); }
	const ColorBlendState& getColorBlendState() const { return state.getColorBlendState(); }
	const std::string& getEntryPoint() const { return state.getEntryPoint(); }
	const ShaderRef& getShader() const { return shader; }
	const FBORef& getFBO() const { return state.getFBO(); }

	void setType(const PipelineType& value) { type = value; }
	PipelineType getType() const { return type; }

	const PipelineHandle& getApiHandle() const { return handle; }
private:
	explicit Pipeline(const DeviceRef& device);

	const DeviceRef device;
	PipelineType type = PipelineType::Graphics;
	PipelineState state;
	ShaderRef shader;
	Ref parent;
	PipelineHandle handle;
	size_t hash = 0;
};

//---------------

} /* Rendering */

#endif /* end of include guard: RENDERING_CORE_PIPELINE_H_ */
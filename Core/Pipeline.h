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
class FBO;
using FBORef = Util::Reference<FBO>;

enum PipelineType {
	Graphics,
	Compute
};

//---------------

class Pipeline : public Util::ReferenceCounter<Pipeline> {
public:
	using Ref = Util::Reference<Pipeline>;
	Pipeline(Pipeline &&) = default;
	Pipeline(const Pipeline &) = delete;
	virtual ~Pipeline();

	bool validate();
	void invalidate() { handle = nullptr; }
	bool isValid() const { return handle; }

	const ShaderRef& getShader() const { return shader; }
	void setShader(const ShaderRef& value);

	PipelineType getType() const { return type; }
	const PipelineHandle& getApiHandle() const { return handle; }

	virtual void reset();
protected:
	explicit Pipeline(const DeviceRef& device, PipelineType type);
private:
	const PipelineType type;
	DeviceRef device;
	PipelineHandle::Ref handle;
	ShaderRef shader;
};

//---------------

class GraphicsPipeline : public Pipeline {
public:
	using Ref = Util::Reference<GraphicsPipeline>;
	static Ref create(const DeviceRef& device, const PipelineState& state, const ShaderRef& shader = nullptr, const FBORef& fbo = nullptr);

	const PipelineState& getState() const { return state; }
	void setState(const PipelineState& value);

	const FBORef& getFBO() const { return fbo; }
	void setFBO(const FBORef& value);

	virtual void reset();
private:
	explicit GraphicsPipeline(const DeviceRef& device) : Pipeline(device, PipelineType::Graphics) {}

	PipelineState state;
	FBORef fbo;
};

//---------------

class ComputePipeline : public Pipeline {
public:
	using Ref = Util::Reference<ComputePipeline>;
	static Ref create(const DeviceRef& device, const ShaderRef& shader = nullptr, const std::string& entryPoint = "main");
	
	const std::string& getEntryPoint() const { return entryPoint; }
	void setEntryPoint(const std::string& value);
	
	virtual void reset();
private:
	explicit ComputePipeline(const DeviceRef& device) : Pipeline(device, PipelineType::Compute) {}
	std::string entryPoint = "main";
};

} /* Rendering */

#endif /* end of include guard: RENDERING_CORE_PIPELINE_H_ */
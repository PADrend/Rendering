/*
	This file is part of the Rendering library.
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_CORE_COMMANDBUFFER_H_
#define RENDERING_CORE_COMMANDBUFFER_H_

#include "Common.h"
#include "../RenderingContext/PipelineState.h"
#include "../RenderingContext/BindingState.h"

#include <Util/ReferenceCounter.h>
#include <Util/Graphics/Color.h>

#include <vector>
#include <deque>
#include <map>
#include <functional>

namespace Rendering {
class CommandPool;
class Texture;
using TextureRef = Util::Reference<Texture>;
class BufferObject;
using BufferObjectRef = Util::Reference<BufferObject>;
class Pipeline;
using PipelineRef = Util::Reference<Pipeline>;
class Device;
using DeviceRef = Util::Reference<Device>;
class DescriptorSet;
using DescriptorSetRef = Util::Reference<DescriptorSet>;

//-------------------------------------------------------

class CommandBuffer : public Util::ReferenceCounter<CommandBuffer> {
public:
	using Ref = Util::Reference<CommandBuffer>;
	enum State {
		Invalid,
		Initial,
		Recording,
		Executable,
		Free,
	};

	static Ref request(const DeviceRef& device, QueueFamily family=QueueFamily::Graphics, bool primary=true);
	
	~CommandBuffer();
	
	void reset();
	void free();
	void flush(PipelineType bindPoint = PipelineType::Graphics);

	void begin();
	void end();

	void beginRenderPass(const std::vector<Util::Color4f>& clearColors);
	void endRenderPass();

	void bindBuffer(const BufferObjectRef& buffer, uint32_t set=0, uint32_t binding=0, uint32_t arrayElement=0);
	void bindTexture(const TextureRef& texture, uint32_t set=0, uint32_t binding=0, uint32_t arrayElement=0);
	void bindInputImage(const ImageViewRef& view, uint32_t set=0, uint32_t binding=0, uint32_t arrayElement=0);

	void draw(uint32_t vertexCount, uint32_t instanceCount=1, uint32_t firstVertex=0, uint32_t firstInstance=0);

	void textureBarrier(const TextureRef& texture, ResourceUsage newUsage);
	//void bufferBarrier(const BufferObjectRef& buffer, ResourceUsage newUsage);

	PipelineState& getPipeline() { return pipelineState; }
	void setPipeline(const PipelineState& value) { pipelineState = value; }
	
	void setVertexInputState(const VertexInputState& state) { pipelineState.setVertexInputState(state); }
	void setInputAssemblyState(const InputAssemblyState& state) { pipelineState.setInputAssemblyState(state); }
	void setViewportState(const ViewportState& state) { pipelineState.setViewportState(state); }
	void setRasterizationState(const RasterizationState& state) { pipelineState.setRasterizationState(state); }
	void setMultisampleState(const MultisampleState& state) { pipelineState.setMultisampleState(state); }
	void setDepthStencilState(const DepthStencilState& state) { pipelineState.setDepthStencilState(state); }
	void setColorBlendState(const ColorBlendState& state) { pipelineState.setColorBlendState(state); }
	void setEntryPoint(const std::string& value) { pipelineState.setEntryPoint(value); }
	void setShader(const ShaderRef& shader) { pipelineState.setShader(shader); }
	void setFBO(const FBORef& fbo) { pipelineState.setFBO(fbo); }
	
	const VertexInputState& getVertexInputState() const { return pipelineState.getVertexInputState(); }
	const InputAssemblyState& getInputAssemblyState() const { return pipelineState.getInputAssemblyState(); }
	const ViewportState& getViewportState() const { return pipelineState.getViewportState(); }
	const RasterizationState& getRasterizationState() const { return pipelineState.getRasterizationState(); }
	const MultisampleState& getMultisampleState() const { return pipelineState.getMultisampleState(); }
	const DepthStencilState& getDepthStencilState() const { return pipelineState.getDepthStencilState(); }
	const ColorBlendState& getColorBlendState() const { return pipelineState.getColorBlendState(); }
	const std::string& getEntryPoint() const { return pipelineState.getEntryPoint(); }
	const ShaderRef& getShader() const { return pipelineState.getShader(); }
	const FBORef& getFBO() const { return pipelineState.getFBO(); }

	State getState() const { return state; }

	bool isRecording() const { return state == State::Recording; }
	bool isExecutable() const { return state == State::Executable; }
	bool isPrimary() const { return primary; }
	const CommandBufferHandle& getApiHandle() const { return handle; };
private:
	friend class CommandPool;
	explicit CommandBuffer(CommandPool* pool, bool primary=true); 
	bool init();

	Util::WeakPointer<CommandPool> pool;
	bool primary;
	CommandBufferHandle handle;
	State state = Invalid;
	PipelineState pipelineState;
	PipelineRef pipeline;
	BindingState bindings;
	std::map<uint32_t, size_t> layoutHashs;
};

} /* Rendering */

#endif /* end of include guard: RENDERING_CORE_COMMANDBUFFER_H_ */

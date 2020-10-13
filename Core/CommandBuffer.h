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
#include "Pipeline.h"
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
class BufferStorage;
using BufferStorageRef = Util::Reference<BufferStorage>;
class DescriptorSet;
using DescriptorSetRef = Util::Reference<DescriptorSet>;
class Queue;
using QueueRef = Util::Reference<Queue>;

//-------------------------------------------------------

class CommandBuffer : public Util::ReferenceCounter<CommandBuffer> {
public:
	using Ref = Util::Reference<CommandBuffer>;
	enum State {
		Invalid,
		Initial,
		Recording,
		Executable,
	};

	static Ref create(const DeviceRef& device, QueueFamily family=QueueFamily::Graphics, bool primary=true);
	static Ref create(const QueueRef& queue, bool primary=true);
	
	~CommandBuffer();
	
	void reset();
	void flush();
	void submit(bool wait=false);

	void begin();
	void end();

	void beginRenderPass(const std::vector<Util::Color4f>& clearColors={});
	void endRenderPass();

	void clearColor(const std::vector<Util::Color4f>& clearColors);

	void bindBuffer(const BufferObjectRef& buffer, uint32_t set=0, uint32_t binding=0, uint32_t arrayElement=0);
	void bindTexture(const TextureRef& texture, uint32_t set=0, uint32_t binding=0, uint32_t arrayElement=0);
	void bindInputImage(const ImageViewRef& view, uint32_t set=0, uint32_t binding=0, uint32_t arrayElement=0);

	void pushConstants(const std::vector<uint8_t>& data, uint32_t offset=0);

	template<typename T>
	void pushConstants(const T& value, uint32_t offset=0) {
		pushConstants({reinterpret_cast<const uint8_t *>(&value), reinterpret_cast<const uint8_t *>(&value) + sizeof(T)}, offset);
	}

	void bindVertexBuffers(uint32_t firstBinding, const std::vector<BufferObjectRef>& buffers, const std::vector<size_t>& offsets={});
	void bindIndexBuffer(const BufferObjectRef& buffer, size_t offset=0);

	void draw(uint32_t vertexCount, uint32_t instanceCount=1, uint32_t firstVertex=0, uint32_t firstInstance=0);

	void copyBuffer(const BufferStorageRef& srcBuffer, const BufferStorageRef& tgtBuffer, size_t size);
	void copyBuffer(const BufferObjectRef& srcBuffer, const BufferObjectRef& tgtBuffer, size_t size);

	void textureBarrier(const TextureRef& texture, ResourceUsage newUsage);
	//void bufferBarrier(const BufferObjectRef& buffer, ResourceUsage newUsage);


	PipelineState& getPipelineState() { return pipeline->getState(); }
	void setPipelineState(const PipelineState& value) { pipeline->setState(value); }
	
	void setVertexInputState(const VertexInputState& state) { pipeline->setVertexInputState(state); }
	void setInputAssemblyState(const InputAssemblyState& state) { pipeline->setInputAssemblyState(state); }
	void setViewportState(const ViewportState& state) { pipeline->setViewportState(state); }
	void setRasterizationState(const RasterizationState& state) { pipeline->setRasterizationState(state); }
	void setMultisampleState(const MultisampleState& state) { pipeline->setMultisampleState(state); }
	void setDepthStencilState(const DepthStencilState& state) { pipeline->setDepthStencilState(state); }
	void setColorBlendState(const ColorBlendState& state) { pipeline->setColorBlendState(state); }
	void setEntryPoint(const std::string& value) { pipeline->setEntryPoint(value); }
	void setShader(const ShaderRef& shader) { pipeline->setShader(shader); }
	void setFBO(const FBORef& fbo) { pipeline->setFBO(fbo); }
	
	const VertexInputState& getVertexInputState() const { return pipeline->getVertexInputState(); }
	const InputAssemblyState& getInputAssemblyState() const { return pipeline->getInputAssemblyState(); }
	const ViewportState& getViewportState() const { return pipeline->getViewportState(); }
	const RasterizationState& getRasterizationState() const { return pipeline->getRasterizationState(); }
	const MultisampleState& getMultisampleState() const { return pipeline->getMultisampleState(); }
	const DepthStencilState& getDepthStencilState() const { return pipeline->getDepthStencilState(); }
	const ColorBlendState& getColorBlendState() const { return pipeline->getColorBlendState(); }
	const std::string& getEntryPoint() const { return pipeline->getEntryPoint(); }
	const ShaderRef& getShader() const { return pipeline->getShader(); }
	const FBORef& getFBO() const { return pipeline->getFBO(); }

	State getState() const { return state; }

	bool isRecording() const { return state == State::Recording; }
	bool isExecutable() const { return state == State::Executable; }
	bool isPrimary() const { return primary; }
	const CommandBufferHandle& getApiHandle() const { return handle; };
private:
	friend class Queue;
	explicit CommandBuffer(const QueueRef& queue, bool primary=true);
	bool init();

	Util::WeakPointer<Queue> queue;
	bool primary;
	CommandBufferHandle handle;
	State state = Invalid;
	Pipeline::Ref pipeline;
	PipelineHandle boundPipeline;
	std::map<uint32_t, DescriptorSetRef> descriptorSets;
	BindingState bindings;
};

} /* Rendering */

#endif /* end of include guard: RENDERING_CORE_COMMANDBUFFER_H_ */

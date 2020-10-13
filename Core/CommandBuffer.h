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
#include "../State/PipelineState.h"
#include "../State/BindingState.h"

#include <Util/ReferenceCounter.h>
#include <Util/Graphics/Color.h>

#include <Geometry/Rect.h>

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
class ImageStorage;
using ImageStorageRef = Util::Reference<ImageStorage>;
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
	
	//! @name Command buffer recording & executing
	//! @{
	void reset();
	void flush();
	void submit(bool wait=false);

	void begin();
	void end();

	void beginRenderPass(const FBORef& fbo=nullptr, bool clearColor=true, bool clearDepth=true, const std::vector<Util::Color4f>& clearColors={}, float clearDepthValue=0, uint32_t clearStencilValue=0);
	void endRenderPass();
	//! @}

	//! @name Binding commands
	//! @{
	void bindBuffer(const BufferObjectRef& buffer, uint32_t set=0, uint32_t binding=0, uint32_t arrayElement=0);
	void bindTexture(const TextureRef& texture, uint32_t set=0, uint32_t binding=0, uint32_t arrayElement=0);
	void bindInputImage(const ImageViewRef& view, uint32_t set=0, uint32_t binding=0, uint32_t arrayElement=0);
	void bindVertexBuffers(uint32_t firstBinding, const std::vector<BufferObjectRef>& buffers, const std::vector<size_t>& offsets={});
	void bindIndexBuffer(const BufferObjectRef& buffer, size_t offset=0);
	void setBindings(const BindingState& state) { bindings = state; }
	//! @}

	//! @name Push constants
	//! @{
	void pushConstants(const uint8_t* data, size_t size, size_t offset=0);
	void pushConstants(const std::vector<uint8_t>& data, size_t offset=0) {
		pushConstants(data.data(), data.size(), offset);
	}
	template<typename T>
	void pushConstants(const T& value, size_t offset=0) {
		pushConstants(reinterpret_cast<const uint8_t*>(&value), sizeof(T), offset);
	}
	//! @}


	//! @name Draw commands
	//! @{
	void clearColor(const std::vector<Util::Color4f>& clearColors, const Geometry::Rect_i& rect={});
	void clearDepthStencil(float depth, uint32_t stencil, const Geometry::Rect_i& rect={}, bool clearDepth=true, bool clearStencil=true);
	void draw(uint32_t vertexCount, uint32_t instanceCount=1, uint32_t firstVertex=0, uint32_t firstInstance=0);
	void drawIndexed(uint32_t indexCount, uint32_t instanceCount=1, uint32_t firstIndex=0, uint32_t vertexOffset=0, uint32_t firstInstance=0);
	void drawIndirect(const BufferObjectRef& buffer, uint32_t drawCount=0, uint32_t stride=0, size_t offset=0);
	void drawIndexedIndirect(const BufferObjectRef& buffer, uint32_t drawCount=0, uint32_t stride=0, size_t offset=0);
	//! @}

	//! @name Copy commands
	//! @{
	void copyBuffer(const BufferStorageRef& srcBuffer, const BufferStorageRef& tgtBuffer, size_t size, size_t srcOffset=0, size_t tgtOffset=0);
	void copyBuffer(const BufferObjectRef& srcBuffer, const BufferObjectRef& tgtBuffer, size_t size, size_t srcOffset=0, size_t tgtOffset=0);
	void updateBuffer(const BufferStorageRef& buffer, const uint8_t* data, size_t size, size_t offset=0);
	void copyImage(const ImageStorageRef& srcImage, const ImageStorageRef& tgtImage, const ImageRegion& srcRegion, const ImageRegion& tgtRegion);
	void copyBufferToImage(const BufferStorageRef& srcBuffer, const ImageStorageRef& tgtImage, size_t srcOffset, const ImageRegion& tgtRegion);
	void copyImageToBuffer(const ImageStorageRef& srcImage, const BufferStorageRef& tgtBuffer, const ImageRegion& srcRegion, size_t tgtOffset);
	void blitImage(const ImageStorageRef& srcImage, const ImageStorageRef& tgtImage, const ImageRegion& srcRegion, const ImageRegion& tgtRegion, ImageFilter filter=ImageFilter::Nearest);
	//! @}

	//! @name Memory barriers
	//! @{
	void textureBarrier(const TextureRef& texture, ResourceUsage newUsage);
	void imageBarrier(const ImageStorageRef& image, ResourceUsage newUsage);
	//void bufferBarrier(const BufferObjectRef& buffer, ResourceUsage newUsage);
	//! @}

	//! @name Pipeline state
	//! @{

	PipelineState& getPipeline() { return pipeline; }
	void setPipeline(const PipelineState& value) { pipeline = value; }
	
	void setVertexInputState(const VertexInputState& state) { pipeline.setVertexInputState(state); }
	void setInputAssemblyState(const InputAssemblyState& state) { pipeline.setInputAssemblyState(state); }
	void setViewportState(const ViewportState& state) { pipeline.setViewportState(state); }
	void setRasterizationState(const RasterizationState& state) { pipeline.setRasterizationState(state); }
	void setMultisampleState(const MultisampleState& state) { pipeline.setMultisampleState(state); }
	void setDepthStencilState(const DepthStencilState& state) { pipeline.setDepthStencilState(state); }
	void setColorBlendState(const ColorBlendState& state) { pipeline.setColorBlendState(state); }
	void setFramebufferFormat(const FramebufferFormat& state) { pipeline.setFramebufferFormat(state); }
	void setFramebufferFormat(const FBORef& fbo) { pipeline.setFramebufferFormat(fbo); }
	void setEntryPoint(const std::string& value) { pipeline.setEntryPoint(value); }
	void setShader(const ShaderRef& shader) { pipeline.setShader(shader); }
	
	const VertexInputState& getVertexInputState() const { return pipeline.getVertexInputState(); }
	const InputAssemblyState& getInputAssemblyState() const { return pipeline.getInputAssemblyState(); }
	const ViewportState& getViewportState() const { return pipeline.getViewportState(); }
	const RasterizationState& getRasterizationState() const { return pipeline.getRasterizationState(); }
	const MultisampleState& getMultisampleState() const { return pipeline.getMultisampleState(); }
	const DepthStencilState& getDepthStencilState() const { return pipeline.getDepthStencilState(); }
	const ColorBlendState& getColorBlendState() const { return pipeline.getColorBlendState(); }
	const FramebufferFormat& getFramebufferFormat() const { return pipeline.getFramebufferFormat(); }
	const std::string& getEntryPoint() const { return pipeline.getEntryPoint(); }
	const ShaderRef& getShader() const { return pipeline.getShader(); }

	//! @}

	//! @name Command buffer state
	//! @{
	bool isRecording() const { return state == State::Recording; }
	bool isExecutable() const { return state == State::Executable; }
	bool isInRenderPass() const { return inRenderPass; }
	bool isPrimary() const { return primary; }
	State getState() const { return state; }
	const FBORef& getActiveFBO() const { return activeFBO; }
	//! @}

	//! @name Internal
	//! @{
	const CommandBufferHandle& getApiHandle() const { return handle; };
	//! @}
private:
	friend class Queue;
	explicit CommandBuffer(const QueueRef& queue, bool primary=true);
	bool init();

	Util::WeakPointer<Queue> queue;
	bool primary;
	CommandBufferHandle handle;
	State state = Invalid;
	bool inRenderPass=false;
	FBORef activeFBO;
	PipelineState pipeline;
	BindingState bindings;

	// Keep as long as command buffer is used
	std::vector<PipelineHandle> boundPipelines;
	std::vector<DescriptorSetRef> boundDescriptorSets;
};

} /* Rendering */

#endif /* end of include guard: RENDERING_CORE_COMMANDBUFFER_H_ */

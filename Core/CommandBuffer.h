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
#include "Commands/Command.h"
#include "../State/PipelineState.h"
#include "../State/BindingState.h"

#include <Util/ReferenceCounter.h>
#include <Util/Graphics/Color.h>

#include <Geometry/Rect.h>

#include <vector>
#include <deque>
#include <map>
#include <functional>
#include <atomic>

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
class ImageView;
using ImageViewRef = Util::Reference<ImageView>;
class DescriptorSet;
using DescriptorSetRef = Util::Reference<DescriptorSet>;
class Queue;
using QueueRef = Util::Reference<Queue>;

//-------------------------------------------------------

class CommandBuffer : public Util::ReferenceCounter<CommandBuffer> {
private:
	explicit CommandBuffer(const QueueRef& queue, bool primary=true, bool transient=true);
public:
	using Ref = Util::Reference<CommandBuffer>;
	enum State {
		Invalid,
		Recording,
		Compiling,
		Executable,
	};

	RENDERINGAPI static Ref create(const DeviceRef& device, QueueFamily family=QueueFamily::Graphics, bool transient=true, bool primary=true);
	RENDERINGAPI static Ref create(const QueueRef& queue, bool transient=true, bool primary=true);
	
	RENDERINGAPI ~CommandBuffer();
	
	//! @name Command buffer recording & executing
	//! @{
	RENDERINGAPI void reset();
	RENDERINGAPI void flush();
	RENDERINGAPI void submit(bool wait=false);
	RENDERINGAPI void execute(const Ref& buffer);
	RENDERINGAPI bool compile(CompileContext& context);
	RENDERINGAPI bool compile();

	RENDERINGAPI void beginRenderPass(const FBORef& fbo=nullptr, bool clearColor=true, bool clearDepth=true, bool clearStencil=true);
	RENDERINGAPI void endRenderPass();
	RENDERINGAPI void prepareForPresent();
	RENDERINGAPI void addCommand(Command* cmd);
	//! @}

	//! @name Binding commands
	//! @{
	RENDERINGAPI void bindBuffer(const BufferObjectRef& buffer, uint32_t set=0, uint32_t binding=0, uint32_t arrayElement=0);
	RENDERINGAPI void bindTexture(const TextureRef& texture, uint32_t set=0, uint32_t binding=0, uint32_t arrayElement=0);
	RENDERINGAPI void bindVertexBuffers(uint32_t firstBinding, const std::vector<BufferObjectRef>& buffers);
	RENDERINGAPI void bindIndexBuffer(const BufferObjectRef& buffer);
	void setBindings(const BindingState& state) { bindings = state; }
	void updateBindings(const BindingState& state) { bindings.merge(state); }
	BindingState& getBindings() { return bindings; }
	//! @}

	//! @name Push constants
	//! @{
	RENDERINGAPI void pushConstants(const uint8_t* data, size_t size, size_t offset=0);
	void pushConstants(const std::vector<uint8_t>& data, size_t offset=0) {
		pushConstants(data.data(), data.size(), offset);
	}
	template<typename T>
	void pushConstants(const T& value, size_t offset=0) {
		pushConstants(reinterpret_cast<const uint8_t*>(&value), sizeof(T), offset);
	}
	//! @}

	//! @name Clear commands
	//! @{
	RENDERINGAPI void clear(bool clearColor=true, bool clearDepth=true, bool clearStencil=true, const Geometry::Rect_i& rect={});
	RENDERINGAPI void setClearColor(const std::vector<Util::Color4f>& colors);
	RENDERINGAPI void setClearDepthValue(float depth);
	RENDERINGAPI void setClearStencilValue(uint32_t stencil);
	RENDERINGAPI void clearColor(const std::vector<Util::Color4f>& colors, const Geometry::Rect_i& rect={});
	RENDERINGAPI void clearDepth(float depth, const Geometry::Rect_i& rect={});
	RENDERINGAPI void clearStencil(uint32_t stencil, const Geometry::Rect_i& rect={});
	RENDERINGAPI void clearDepthStencil(float depth, uint32_t stencil, const Geometry::Rect_i& rect={});
	RENDERINGAPI void clearImage(const TextureRef& texture, const Util::Color4f& color);
	RENDERINGAPI void clearImage(const ImageViewRef& view, const Util::Color4f& color);
	RENDERINGAPI void clearImage(const ImageStorageRef& image, const Util::Color4f& color);
	//! @}

	//! @name Draw commands
	//! @{
	RENDERINGAPI void draw(uint32_t vertexCount, uint32_t instanceCount=1, uint32_t firstVertex=0, uint32_t firstInstance=0);
	RENDERINGAPI void drawIndexed(uint32_t indexCount, uint32_t instanceCount=1, uint32_t firstIndex=0, uint32_t vertexOffset=0, uint32_t firstInstance=0);
	RENDERINGAPI void drawIndirect(const BufferObjectRef& buffer, uint32_t drawCount=0, uint32_t stride=0, size_t offset=0);
	RENDERINGAPI void drawIndexedIndirect(const BufferObjectRef& buffer, uint32_t drawCount=0, uint32_t stride=0, size_t offset=0);
	//! @}

	//! @name Copy commands
	//! @{
	RENDERINGAPI void copyBuffer(const BufferStorageRef& srcBuffer, const BufferStorageRef& tgtBuffer, size_t size, size_t srcOffset=0, size_t tgtOffset=0);
	RENDERINGAPI void copyBuffer(const BufferObjectRef& srcBuffer, const BufferObjectRef& tgtBuffer, size_t size, size_t srcOffset=0, size_t tgtOffset=0);
	RENDERINGAPI void updateBuffer(const BufferStorageRef& buffer, const uint8_t* data, size_t size, size_t offset=0);
	RENDERINGAPI void copyImage(const ImageStorageRef& srcImage, const ImageStorageRef& tgtImage, const ImageRegion& srcRegion, const ImageRegion& tgtRegion);
	RENDERINGAPI void copyBufferToImage(const BufferStorageRef& srcBuffer, const ImageStorageRef& tgtImage, size_t srcOffset, const ImageRegion& tgtRegion);
	RENDERINGAPI void copyImageToBuffer(const ImageStorageRef& srcImage, const BufferStorageRef& tgtBuffer, const ImageRegion& srcRegion, size_t tgtOffset);
	RENDERINGAPI void blitImage(const ImageStorageRef& srcImage, const ImageStorageRef& tgtImage, const ImageRegion& srcRegion, const ImageRegion& tgtRegion, ImageFilter filter=ImageFilter::Nearest);
	//! @}

	//! @name Memory barriers
	//! @{
	RENDERINGAPI void imageBarrier(const TextureRef& texture, ResourceUsage newUsage);
	RENDERINGAPI void imageBarrier(const ImageStorageRef& image, ResourceUsage newUsage);
	RENDERINGAPI void imageBarrier(const ImageViewRef& image, ResourceUsage newUsage);
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
	RENDERINGAPI void setFBO(const FBORef& fbo);
	
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
	const FBORef& getFBO() const { return activeFBO; }
	//! @}


	//! @name Dynamic state
	//! @{
	RENDERINGAPI void setScissor(const Geometry::Rect_i& scissor);
	RENDERINGAPI void setLineWidth(float width);
	//! @}


	//! @name Command buffer state
	//! @{
	bool isRecording() const { return state == State::Recording; }
	bool isExecutable() const { return state == State::Executable; }
	bool isInRenderPass() const { return inRenderPass; }
	bool isPrimary() const { return primary; }
	State getState() const { return state; }
	uint32_t getCommandCount() const { return static_cast<uint32_t>(commands.size()); }
	//! @}

	//! @name Debugging
	//! @{
	void beginDebugMarker(const std::string& name, const Util::Color4f& color={});
	void insertDebugMarker(const std::string& name, const Util::Color4f& color={});
	RENDERINGAPI void endDebugMarker();
	RENDERINGAPI void setDebugName(const std::string& name);
	//! @}

	//! @name Internal
	//! @{
	const CommandBufferHandle& getApiHandle() const { return handle; };
	const SemaphoreHandle& getSignalSemaphore() const { return signalSemaphore; };
	QueueRef getQueue() const { return queue.get(); };
	//! @}
private:
	void ensureRenderPass() {
		if(!inRenderPass)
			beginRenderPass(activeFBO, false, false, false);
	}

	Util::WeakPointer<Queue> queue;
	bool primary;
	bool transient;
	CommandBufferHandle handle;
	std::deque<Command::Ptr> commands;

	std::atomic<State> state;
	bool inRenderPass=false;
	FBORef activeFBO;
	PipelineState pipeline;
	BindingState bindings;

	std::vector<Util::Color4f> clearColors;
	float clearDepthValue=1;
	uint32_t clearStencilValue=0;

	SemaphoreHandle signalSemaphore;
	uint32_t ownerId=0;
};

} /* Rendering */

#endif /* end of include guard: RENDERING_CORE_COMMANDBUFFER_H_ */

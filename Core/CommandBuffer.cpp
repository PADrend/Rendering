/*
	This file is part of the Rendering library.
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "CommandBuffer.h"
#include "Device.h"
#include "ResourceCache.h"
#include "DescriptorPool.h"
#include "ImageStorage.h"
#include "ImageView.h"
#include "BufferStorage.h"
#include "Swapchain.h"
#include "../Shader/Shader.h"
#include "../Texture/Texture.h"
#include "../BufferObject.h"
#include "../FBO.h"
#include "Commands/BindCommands.h"
#include "Commands/CommonCommands.h"
#include "Commands/CopyCommands.h"
#include "Commands/DrawCommands.h"
#include "Commands/DynamicStateCommands.h"

#include <Util/Macros.h>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#include <unordered_set>
#include <algorithm>

#define PROFILING_ENABLED 1
#ifdef PROFILING_ENABLED
#include <Util/Profiling/Profiler.h>
#include <Util/Profiling/Logger.h>
#include <fstream>

static std::ofstream _out("CmdLog.txt", std::fstream::out);
INIT_PROFILING_TIME(_out);
#endif

namespace Rendering {

//-----------------

vk::AccessFlags getVkAccessMask(const ResourceUsage& usage);
vk::ImageLayout getVkImageLayout(const ResourceUsage& usage);
vk::PipelineStageFlags getVkPipelineStageMask(const ResourceUsage& usage, bool src);
vk::Filter getVkFilter(const ImageFilter& filter);

//-----------------

CommandBuffer::Ref CommandBuffer::create(const DeviceRef& device, QueueFamily family, bool transient, bool primary) {
	return create(device->getQueue(family), transient, primary);
}

//-----------------

CommandBuffer::Ref CommandBuffer::create(const QueueRef& queue, bool transient, bool primary) {
	auto buffer = new CommandBuffer(queue, primary, transient);
	return buffer;
}

//-----------------

CommandBuffer::CommandBuffer(const QueueRef& queue, bool primary, bool transient) : queue(queue.get()), primary(primary), transient(transient), state(State::Recording) {
	reset();
}

//-----------------

CommandBuffer::~CommandBuffer() {
	if(handle)
		queue->freeCommandBuffer(handle, primary, ownerId);
};

//-----------------

bool CommandBuffer::compile(CompileContext& context) {
	#ifdef PROFILING_ENABLED
		static uint64_t _profilingCount = 0;
		++_profilingCount;
	#endif
	if(state == State::Executable)
		return true;
	endRenderPass();
	WARN_AND_RETURN_IF(state == State::Compiling, "Command Buffer is already compiling.", false);
	ownerId = context.ownerId;
	handle = queue->requestCommandBuffer(primary, context.ownerId);
	WARN_AND_RETURN_IF(!handle || state == State::Invalid, "Could not compile command buffer: Invalid command buffer.", false);
	WARN_AND_RETURN_IF(commands.empty(), "Could not compile command buffer: Command list is empty.", false);
	state = State::Compiling;

	context.cmd = handle;
	if(!context.device || !context.descriptorPool || !context.resourceCache) {
		context.device = queue->getDevice();
		context.descriptorPool = context.device->getDescriptorPool();
		context.resourceCache = context.device->getResourceCache();
	}

	vk::CommandBuffer vkCmd(handle);
	vkCmd.begin({transient ? vk::CommandBufferUsageFlagBits::eOneTimeSubmit : vk::CommandBufferUsageFlagBits::eSimultaneousUse});
	for(auto& cmd : commands) {
		#ifdef PROFILING_ENABLED
			Util::Profiling::ScopedAction _action((_profilingCount==1000) ? &_profiler : nullptr, cmd->getTypeName());
		#endif
		if(!cmd->compile(context)) {
			WARN("Failed to compile command buffer.");
			state = State::Invalid;
			return false;
		}
	}
	vkCmd.end();
	state = State::Executable;
	return true;
}

//-----------------

bool CommandBuffer::compile() {
	if(state == State::Executable)
		return true;
	CompileContext context{};
	return compile(context);
}

//-----------------

void CommandBuffer::reset() {
	WARN_AND_RETURN_IF(state == State::Compiling, "Cannot reset command buffer: Command buffer is compiling.",);
	endRenderPass();
	if(handle) {
		queue->freeCommandBuffer(handle, primary, ownerId);
		handle = nullptr;
	}
	pipeline.reset();
	bindings.reset();
	commands.clear();
	vk::Device vkDevice(queue->getApiHandle());
	signalSemaphore = SemaphoreHandle::create(vkDevice.createSemaphore({}), vkDevice);
	state = State::Recording;
}

//-----------------

void CommandBuffer::flush() {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording.",);
	const auto& shader = pipeline.getShader();
	WARN_AND_RETURN_IF(!shader, "Cannot flush command buffer. Invalid shader.",);
	const auto& layout = shader->getLayout();

	if(pipeline.isDirty()) {
		commands.emplace_back(new BindPipelineCommand(pipeline));
		if(pipeline.getViewportState().hasDynamicScissors() && pipeline.getViewportState().isDirty())
			setScissor(pipeline.getViewportState().getScissor());
		pipeline.clearDirty();
	}

	// update descriptor sets
	if(bindings.isDirty()) {
		bool bindingsChanged = false;
		std::vector<vk::DescriptorSet> bindSets;
		for(auto& it : bindings.getBindingSets()) {
			auto set = it.first;
			auto& bindingSet = it.second;
			if(!bindingSet.isDirty())
				continue;
			if(!layout.hasLayoutSet(set))
				continue;
			
			bindingsChanged = true;
			commands.emplace_back(new BindSetCommand(set, bindingSet, layout, pipeline.getType()));
		}
		bindings.clearDirty();
		if(bindingsChanged) 
			insertDebugMarker("Bindings changed");
	}
}

//-----------------

void CommandBuffer::submit(bool wait) {
	WARN_AND_RETURN_IF(!primary, "Cannot submit secondary command buffer.",);
	if(compile()) {
		queue->submit(this);
		if(wait) queue->wait();
	}
}

//-----------------

void CommandBuffer::execute(const Ref& buffer) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording.",);
	WARN_AND_RETURN_IF(buffer->primary, "Cannot execute primary command buffer as secondary.",);
	WARN_AND_RETURN_IF(!primary, "Cannot execute command buffer on secondary command buffer.",);
	commands.emplace_back(new ExecuteCommandBufferCommand(buffer));
}

//-----------------

void CommandBuffer::beginRenderPass(const FBORef& fbo, bool clearColor, bool clearDepth, bool clearStencil) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording. Call begin() first.",);
	WARN_AND_RETURN_IF(inRenderPass, "Command buffer is already in a render pass. Call endRenderPass() first.",);
	vk::CommandBuffer vkCmd(handle);
	auto device = queue->getDevice();
	activeFBO = fbo ? fbo : device->getSwapchain()->getCurrentFBO();
	pipeline.setFramebufferFormat(activeFBO);
	beginDebugMarker("Render Pass");

	commands.emplace_back(new BeginRenderPassCommand(activeFBO, clearColors, clearDepthValue, clearStencilValue, clearColor, clearDepth, clearStencil));
	inRenderPass = true;
}

//-----------------

void CommandBuffer::endRenderPass() {
	if(!inRenderPass)
		return;
	
	for(uint32_t i=0; i<activeFBO->getColorAttachmentCount(); ++i) {
		const auto& attachment = activeFBO->getColorAttachment(i);
		if(attachment)
			attachment->getImageView()->_setLastUsage(ResourceUsage::RenderTarget);
	}
	const auto& depthAttachment = activeFBO->getDepthStencilAttachment();
	if(depthAttachment)
		depthAttachment->getImageView()->_setLastUsage(ResourceUsage::DepthStencil);

	commands.emplace_back(new EndRenderPassCommand);
	inRenderPass = false;
	endDebugMarker();
}

//-----------------

void CommandBuffer::bindBuffer(const BufferObjectRef& buffer, uint32_t set, uint32_t binding, uint32_t arrayElement) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording.",);
	bindings.bind(buffer, set, binding, arrayElement);
}

//-----------------

void CommandBuffer::bindTexture(const TextureRef& texture, uint32_t set, uint32_t binding, uint32_t arrayElement) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording.",);
	bindings.bind(texture, set, binding, arrayElement);
}

//-----------------

void CommandBuffer::pushConstants(const uint8_t* data, size_t size, size_t offset) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording.",);
	const auto& shader = pipeline.getShader();
	WARN_AND_RETURN_IF(!shader, "Cannot set push constants. No bound shader.",);
	const auto& layout = shader->getLayout();
	commands.emplace_back(new PushConstantCommand(data, size, offset, layout));
}

//-----------------

void CommandBuffer::bindVertexBuffers(uint32_t firstBinding, const std::vector<BufferObjectRef>& buffers) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording.",);
	commands.emplace_back(new BindVertexBuffersCommand(firstBinding, buffers));
}

//-----------------

void CommandBuffer::bindIndexBuffer(const BufferObjectRef& buffer) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording.",);
	commands.emplace_back(new BindIndexBufferCommand(buffer));
}

//-----------------

void CommandBuffer::clear(bool clearColor, bool clearDepth, bool clearStencil, const Geometry::Rect_i& rect) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording.",);
	if(!inRenderPass) {
		beginRenderPass(activeFBO, clearColor, clearDepth, clearStencil);
	} else {
		WARN_AND_RETURN_IF(!activeFBO || !activeFBO->isValid(), "Cannot clear attachments. Invalid FBO.",);
		std::vector<Util::Color4f> colors(clearColors.begin(), clearColors.end());
		colors.resize(activeFBO->getColorAttachmentCount(), {0,0,0,0});
		Geometry::Rect_i clearRect(
			rect.getX(),
			rect.getY(),
			rect.getWidth() > 0 ? rect.getWidth() : activeFBO->getWidth(),
			rect.getHeight() > 0 ? rect.getHeight() : activeFBO->getHeight()
		);
		commands.emplace_back(new ClearAttachmentsCommand(colors, clearDepthValue, clearStencilValue, clearColor, clearDepth, clearStencil, clearRect));
	}
}

//-----------------

void CommandBuffer::setClearColor(const std::vector<Util::Color4f>& colors) {
	clearColors = colors;
}

//-----------------

void CommandBuffer::setClearDepthValue(float depth) {
	clearDepthValue = depth;
}

//-----------------

void CommandBuffer::setClearStencilValue(uint32_t stencil) {
	clearStencilValue = stencil;
}

//-----------------

void CommandBuffer::clearColor(const std::vector<Util::Color4f>& colors, const Geometry::Rect_i& rect) {
	setClearColor(colors);
	clear(true,false,false,rect);
}

//-----------------

void CommandBuffer::clearDepth(float depth, const Geometry::Rect_i& rect) {
	setClearDepthValue(depth);
	clear(false,true,false,rect);
}

//-----------------

void CommandBuffer::clearStencil(uint32_t stencil, const Geometry::Rect_i& rect) {
	setClearStencilValue(stencil);
	clear(false,false,true,rect);
}

//-----------------

void CommandBuffer::clearDepthStencil(float depth, uint32_t stencil, const Geometry::Rect_i& rect) {
	setClearDepthValue(depth);
	setClearStencilValue(stencil);
	clear(false,true,true,rect);
}

//-----------------

void CommandBuffer::clearImage(const TextureRef& texture, const Util::Color4f& color) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording.",);
	WARN_AND_RETURN_IF(!texture, "Cannot clear image. Invalid texture.",);
	commands.emplace_back(new ClearImageCommand(texture, color));
}

//-----------------

void CommandBuffer::clearImage(const ImageViewRef& view, const Util::Color4f& color) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording.",);
	WARN_AND_RETURN_IF(!view, "Cannot clear image. Invalid image.",);
	commands.emplace_back(new ClearImageCommand(view, color));
}

//-----------------

void CommandBuffer::clearImage(const ImageStorageRef& image, const Util::Color4f& color) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording.",);
	WARN_AND_RETURN_IF(!image, "Cannot clear image. Invalid image.",);
	commands.emplace_back(new ClearImageCommand(image, color));
}

//-----------------

void CommandBuffer::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording.",);
	if(instanceCount==0) return;
	ensureRenderPass();
	pipeline.setType(PipelineType::Graphics); // ensure we have a graphics pipeline
	flush(); // update pipeline & binding state
	commands.emplace_back(new DrawCommand(vertexCount, instanceCount, firstVertex, firstInstance));
}

//-----------------

void CommandBuffer::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording.",);
	if(instanceCount==0) return;
	ensureRenderPass();
	pipeline.setType(PipelineType::Graphics); // ensure we have a graphics pipeline
	flush(); // update pipeline & binding state
	commands.emplace_back(new DrawIndexedCommand(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance));
}

//-----------------

void CommandBuffer::drawIndirect(const BufferObjectRef& buffer, uint32_t drawCount, uint32_t stride, size_t offset) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording.",);
	ensureRenderPass();
	pipeline.setType(PipelineType::Graphics); // ensure we have a graphics pipeline
	flush(); // update pipeline & binding state
	commands.emplace_back(new DrawIndirectCommand(buffer, drawCount, stride, offset));
}

//-----------------

void CommandBuffer::drawIndexedIndirect(const BufferObjectRef& buffer, uint32_t drawCount, uint32_t stride, size_t offset) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording.",);
	ensureRenderPass();
	pipeline.setType(PipelineType::Graphics); // ensure we have a graphics pipeline
	flush(); // update pipeline & binding state
	commands.emplace_back(new DrawIndexedIndirectCommand(buffer, drawCount, stride, offset));
}

//-----------------


void CommandBuffer::copyBuffer(const BufferStorageRef& srcBuffer, const BufferStorageRef& tgtBuffer, size_t size, size_t srcOffset, size_t tgtOffset) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording.",);
	WARN_AND_RETURN_IF(!srcBuffer || !tgtBuffer, "Cannot copy buffer. Invalid buffers.",);
	endRenderPass(); // not allowed in render pass
	commands.emplace_back(new CopyBufferCommand(srcBuffer, tgtBuffer, size, srcOffset, tgtOffset));
}

//-----------------

void CommandBuffer::copyBuffer(const BufferObjectRef& srcBuffer, const BufferObjectRef& tgtBuffer, size_t size, size_t srcOffset, size_t tgtOffset) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording.",);
	WARN_AND_RETURN_IF(!srcBuffer || !tgtBuffer, "Cannot copy buffer. Invalid buffers.",);
	endRenderPass(); // not allowed in render pass
	commands.emplace_back(new CopyBufferCommand(srcBuffer, tgtBuffer, size, srcOffset, tgtOffset));
}

//-----------------

void CommandBuffer::updateBuffer(const BufferStorageRef& buffer, const uint8_t* data, size_t size, size_t offset) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording.",);
	WARN_AND_RETURN_IF(!buffer || !data, "Cannot update buffer. Invalid buffer or data.",);
	endRenderPass(); // not allowed in render pass
	commands.emplace_back(new UpdateBufferCommand(buffer, data, size, offset));
}

//-----------------

void CommandBuffer::copyImage(const ImageStorageRef& srcImage, const ImageStorageRef& tgtImage, const ImageRegion& srcRegion, const ImageRegion& tgtRegion) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording.",);
	WARN_AND_RETURN_IF(!srcImage || !tgtImage, "Cannot copy image. Invalid images.",);
	WARN_AND_RETURN_IF(srcRegion.extent != tgtRegion.extent, "Cannot copy image. Source and target extent must be the same.",);
	endRenderPass(); // not allowed in render pass
	imageBarrier(srcImage, ResourceUsage::CopySource);
	imageBarrier(tgtImage, ResourceUsage::CopyDestination);
	commands.emplace_back(new CopyImageCommand(srcImage, tgtImage, srcRegion, tgtRegion));
}

//-----------------

void CommandBuffer::copyBufferToImage(const BufferStorageRef& srcBuffer, const ImageStorageRef& tgtImage, size_t srcOffset, const ImageRegion& tgtRegion) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording.",);
	WARN_AND_RETURN_IF(!srcBuffer || !tgtImage, "Cannot copy buffer to image. Invalid buffer or image.",);
	endRenderPass(); // not allowed in render pass
	imageBarrier(tgtImage, ResourceUsage::CopyDestination);
	commands.emplace_back(new CopyBufferToImageCommand(srcBuffer, tgtImage, srcOffset, tgtRegion));
}

//-----------------

void CommandBuffer::copyImageToBuffer(const ImageStorageRef& srcImage, const BufferStorageRef& tgtBuffer, const ImageRegion& srcRegion, size_t tgtOffset) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording.",);
	WARN_AND_RETURN_IF(!srcImage || !tgtBuffer, "Cannot copy image to buffer. Invalid buffer or image.",);
	endRenderPass(); // not allowed in render pass
	imageBarrier(srcImage, ResourceUsage::CopySource);
	commands.emplace_back(new CopyImageToBufferCommand(srcImage, tgtBuffer, srcRegion, tgtOffset));
}

//-----------------

void CommandBuffer::blitImage(const ImageStorageRef& srcImage, const ImageStorageRef& tgtImage, const ImageRegion& srcRegion, const ImageRegion& tgtRegion, ImageFilter filter) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording.",);
	WARN_AND_RETURN_IF(!srcImage || !tgtImage, "Cannot blit image. Invalid images.",);
	endRenderPass(); // not allowed in render pass
	imageBarrier(srcImage, ResourceUsage::CopySource);
	imageBarrier(tgtImage, ResourceUsage::CopyDestination);
	commands.emplace_back(new BlitImageCommand(srcImage, tgtImage, srcRegion, tgtRegion, filter));
}

//-----------------

void CommandBuffer::imageBarrier(const TextureRef& texture, ResourceUsage newUsage) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording.",);
	WARN_AND_RETURN_IF(!texture || !texture->isValid(), "Cannot create image barrier. Invalid texture.",);
	if(texture->getLastUsage() == newUsage) return;
	commands.emplace_back(new ImageBarrierCommand(texture, texture->getLastUsage(), newUsage));
	texture->getImageView()->_setLastUsage(newUsage);
}

//-----------------

void CommandBuffer::imageBarrier(const ImageViewRef& view, ResourceUsage newUsage) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording.",);
	WARN_AND_RETURN_IF(!view, "Cannot create image barrier. Invalid image.",);
	if(view->getLastUsage() == newUsage) return;
	commands.emplace_back(new ImageBarrierCommand(view, view->getLastUsage(), newUsage));
	view->_setLastUsage(newUsage);
}

//-----------------

void CommandBuffer::imageBarrier(const ImageStorageRef& image, ResourceUsage newUsage) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording. Call begin() first.",);
	WARN_AND_RETURN_IF(!image, "Cannot create image barrier. Invalid image.",);
	if(image->getLastUsage() == newUsage) return;
	commands.emplace_back(new ImageBarrierCommand(image, image->getLastUsage(), newUsage));
	image->_setLastUsage(newUsage);
}

//-----------------

void CommandBuffer::setScissor(const Geometry::Rect_i& scissor) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording.",);
	if(pipeline.getViewportState().hasDynamicScissors())
		commands.emplace_back(new DynamicScissorCommand({scissor}));
	else
		pipeline.getViewportState().setScissor(scissor);
}

//-----------------

void CommandBuffer::beginDebugMarker(const std::string& name, const Util::Color4f& color) {
	if(!queue->getDevice()->isDebugModeEnabled()) return;
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording.",);
	commands.emplace_back(new DebugMarkerCommand(name, color, DebugMarkerCommand::Begin));
}

//-----------------

void CommandBuffer::insertDebugMarker(const std::string& name, const Util::Color4f& color) {
	if(!queue->getDevice()->isDebugModeEnabled()) return;
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording.",);
	commands.emplace_back(new DebugMarkerCommand(name, color, DebugMarkerCommand::Insert));
}

//-----------------

void CommandBuffer::endDebugMarker() {
	if(!queue->getDevice()->isDebugModeEnabled()) return;
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording.",);
	commands.emplace_back(new DebugMarkerCommand("", {}, DebugMarkerCommand::End));
}

//-------------

void CommandBuffer::setDebugName(const std::string& name) {
	if(!queue->getDevice()->isDebugModeEnabled() || !handle)
		return;
	vk::Device vkDevice(queue->getDevice()->getApiHandle());
	vkDevice.setDebugUtilsObjectNameEXT({ vk::CommandBuffer::objectType, handle, name.c_str() });
}

//-----------------
} // namespace Rendering

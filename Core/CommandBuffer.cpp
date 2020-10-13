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

#include <Util/Macros.h>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#include <unordered_set>
#include <algorithm>

namespace Rendering {

//-----------------

vk::AccessFlags getVkAccessMask(const ResourceUsage& usage);
vk::ImageLayout getVkImageLayout(const ResourceUsage& usage);
vk::PipelineStageFlags getVkPipelineStageMask(const ResourceUsage& usage, bool src);
vk::ShaderStageFlags getVkStageFlags(const ShaderStage& stages);
vk::Filter getVkFilter(const ImageFilter& filter);

//-----------------

static inline Geometry::Vec3i toVec3i(const Geometry::Vec3ui& v) {
	return Geometry::Vec3i(
		static_cast<int32_t>(v.x()),
		static_cast<int32_t>(v.y()),
		static_cast<int32_t>(v.z())
	);
}

//-----------------

CommandBuffer::Ref CommandBuffer::create(const DeviceRef& device, QueueFamily family, bool transient, bool primary) {
	return create(device->getQueue(family), transient, primary);
}

//-----------------

CommandBuffer::Ref CommandBuffer::create(const QueueRef& queue, bool transient, bool primary) {
	auto buffer = new CommandBuffer(queue, primary, transient);
	if(!buffer->init())
		return nullptr;
	return buffer;
}

//-----------------

CommandBuffer::CommandBuffer(const QueueRef& queue, bool primary, bool transient) : queue(queue.get()), primary(primary), transient(transient) { }

//-----------------

CommandBuffer::~CommandBuffer() {
	if(handle)
		queue->freeCommandBuffer(handle, primary);
};

//-----------------

bool CommandBuffer::init() {
	handle = queue->requestCommandBuffer(primary);
	if(handle) {
		state = State::Initial;
		begin();
	}
	return handle.isNotNull();
}

//-----------------

void CommandBuffer::reset() {
	end();
	vk::CommandBuffer vkCmd(handle);
	vkCmd.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
	pipeline.reset();
	boundDescriptorSets.clear();
	boundPipelines.clear();
	boundBuffers.clear();
	boundResource.clear();
	state = State::Initial;
}

//-----------------

void CommandBuffer::flush() {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording. Call begin() first.",);
	vk::CommandBuffer vkCmd(handle);
	auto device = queue->getDevice();
	auto shader = pipeline.getShader();
	WARN_AND_RETURN_IF(!shader, "Cannot flush command buffer. Invalid shader.",);
	auto layout = shader->getLayout();

	// bind pipeline if it has changed
	vk::PipelineLayout vkPipelineLayout(shader->getLayoutHandle());
	vk::PipelineBindPoint vkBindPoint;
	switch (pipeline.getType()) {
		case PipelineType::Compute: vkBindPoint = vk::PipelineBindPoint::eCompute; break;
		case PipelineType::Graphics: vkBindPoint = vk::PipelineBindPoint::eGraphics; break;
	}

	if(boundPipelines.empty() || pipeline.hasChanged()) {
		// pipeline changed
		insertDebugMarker("Pipeline changed");
		PipelineHandle pipelineHandle = device->getResourceCache()->createPipeline(pipeline, nullptr);
		WARN_AND_RETURN_IF(!pipelineHandle, "CommandBuffer: Invalid Pipeline.",);

		vk::Pipeline vkPipeline(pipelineHandle);
		vkCmd.bindPipeline(vkBindPoint, vkPipeline);
		boundPipelines.emplace_back(pipelineHandle);
		pipeline.markAsUnchanged();
	}

	// update descriptor sets
	if(bindings.isDirty()) {
		bindings.clearDirty();
		insertDebugMarker("Bindings changed");

		std::vector<vk::DescriptorSet> bindSets;
		for(auto& it : bindings.getBindingSets()) {
			auto set = it.first;
			auto& bindingSet = it.second;
			if(!bindingSet.isDirty())
				continue;
			bindings.clearDirty(set);
			if(!layout.hasLayoutSet(set))
				continue;
						
			auto descriptorSet = device->getDescriptorPool()->requestDescriptorSet(layout.getLayoutSet(set), bindingSet);
			if(descriptorSet) {
				vk::DescriptorSet vkDescriptorSet(descriptorSet->getApiHandle());
				vkCmd.bindDescriptorSets(vkBindPoint, vkPipelineLayout, set, {vkDescriptorSet}, descriptorSet->getDynamicOffsets());
				boundDescriptorSets.emplace_back(descriptorSet);
			} else {
				WARN("Failed to create descriptor set for binding set " + std::to_string(set));
			}
		}
	}
}

//-----------------

void CommandBuffer::begin() {
	WARN_AND_RETURN_IF(state == State::Recording, "Command buffer is already recording.",);
	WARN_AND_RETURN_IF(state == State::Invalid, "Invalid command buffer.",);
	reset();
	vk::CommandBuffer vkCmd(handle);
	state = State::Recording;
	vkCmd.begin({transient ? vk::CommandBufferUsageFlagBits::eOneTimeSubmit : vk::CommandBufferUsageFlagBits::eSimultaneousUse});
}

//-----------------

void CommandBuffer::end() {
	if(state != State::Recording)
		return;
	endRenderPass();
	vk::CommandBuffer vkCmd(handle);
	state = State::Executable;
	vkCmd.end();
}

//-----------------

void CommandBuffer::submit(bool wait) {
	WARN_AND_RETURN_IF(!primary, "Cannot submit secondary command buffer.",);
	end();
	vk::CommandBuffer vkCmd(handle);
	queue->submit(this, wait);
}

//-----------------

void CommandBuffer::execute(const Ref& buffer) {
	WARN_AND_RETURN_IF(!buffer || !buffer->getApiHandle(), "Cannot execute secondary command buffer. Invalid command buffer.",);
	WARN_AND_RETURN_IF(buffer->primary, "Cannot execute primary command buffer as secondary.",);
	WARN_AND_RETURN_IF(!primary, "Cannot execute command buffer on secondary command buffer.",);
	buffer->end();
	flush();
	vk::CommandBuffer vkCmd(handle);
	vk::CommandBuffer vkSecondaryCmd(buffer->getApiHandle());
	vkCmd.executeCommands(vkSecondaryCmd);
}

//-----------------

void CommandBuffer::beginRenderPass(const FBORef& fbo, bool clearColor, bool clearDepth, bool clearStencil) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording. Call begin() first.",);
	WARN_AND_RETURN_IF(inRenderPass, "Command buffer is already in a render pass. Call endRenderPass() first.",);
	vk::CommandBuffer vkCmd(handle);
	auto device = queue->getDevice();
	activeFBO = fbo ? fbo : device->getSwapchain()->getCurrentFBO();
	pipeline.setFramebufferFormat(activeFBO);

	auto renderPass = device->getResourceCache()->createRenderPass(activeFBO, clearColor, clearDepth, clearStencil);
	auto framebuffer = device->getResourceCache()->createFramebuffer(activeFBO, renderPass);
	WARN_AND_RETURN_IF(!framebuffer, "Failed to start render pass. Invalid framebuffer.",);

	vk::RenderPass vkRenderPass(renderPass);
	vk::Framebuffer vkFramebuffer(framebuffer);

	std::vector<vk::ClearValue> clearValues(activeFBO->getColorAttachmentCount(), vk::ClearColorValue{});
	for(uint32_t i=0; i<std::min<size_t>(clearValues.size(), clearColors.size()); ++i) {
		auto& c = clearColors[i];
		clearValues[i].color.setFloat32({c.r(), c.g(), c.b(), c.a()});
	}
	clearValues.emplace_back(vk::ClearDepthStencilValue(clearDepthValue, clearStencilValue));

	beginDebugMarker("Begin render pass");
	vkCmd.beginRenderPass({
		vkRenderPass, vkFramebuffer,
		vk::Rect2D{ {0, 0}, {activeFBO->getWidth(), activeFBO->getHeight()} },
		static_cast<uint32_t>(clearValues.size()), clearValues.data()
	}, vk::SubpassContents::eInline);
	inRenderPass = true;
}

//-----------------

void CommandBuffer::endRenderPass() {
	if(!inRenderPass)
		return;
	vk::CommandBuffer vkCmd(handle);
	for(uint32_t i=0; i<activeFBO->getColorAttachmentCount(); ++i) {
		const auto& attachment = activeFBO->getColorAttachment(i);
		if(attachment)
			attachment->getImageView()->_setLastUsage(ResourceUsage::RenderTarget);
	}
	const auto& depthAttachment = activeFBO->getDepthStencilAttachment();
	if(depthAttachment)
		depthAttachment->getImageView()->_setLastUsage(ResourceUsage::DepthStencil);

	vkCmd.endRenderPass();
	inRenderPass = false;
	endDebugMarker();
}

//-----------------

void CommandBuffer::prepareForPresent() {
	endRenderPass();
	// TODO: We cannot guarantee that this command buffer uses the same FBO
	auto& fbo = queue->getDevice()->getSwapchain()->getCurrentFBO();
	auto att = fbo->getColorAttachment(0);
	// explicitely transfer image to present layout
	imageBarrier(att, ResourceUsage::Present);
}

//-----------------

void CommandBuffer::bindBuffer(const BufferObjectRef& buffer, uint32_t set, uint32_t binding, uint32_t arrayElement) {
	bindings.bindBuffer(buffer, set, binding, arrayElement);
	boundBuffers.emplace_back(buffer);
}

//-----------------

void CommandBuffer::bindTexture(const TextureRef& texture, uint32_t set, uint32_t binding, uint32_t arrayElement) {
	bindings.bindTexture(texture, set, binding, arrayElement);
}

//-----------------

void CommandBuffer::bindInputImage(const ImageViewRef& view, uint32_t set, uint32_t binding, uint32_t arrayElement) {
	bindings.bindInputImage(view, set, binding, arrayElement);
}

//-----------------

void CommandBuffer::pushConstants(const uint8_t* data, size_t size, size_t offset) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording. Call begin() first.",);
	WARN_AND_RETURN_IF(size+offset > queue->getDevice()->getMaxPushConstantSize(), "Push constant size exceeds maximum size",);
	vk::CommandBuffer vkCmd(handle);
	Shader::Ref shader = pipeline.getShader();
	WARN_AND_RETURN_IF(!shader || !shader->init(), "Cannot set push constants. No bound shader.",);
	auto& layout = shader->getLayout();
	vk::PipelineLayout vkLayout(shader->getLayoutHandle());
	vk::ShaderStageFlags stages{};
	for(auto& range : layout.getPushConstantRanges()) {
		if(offset >= range.offset && offset + size <= range.offset + range.size) {
			stages |= getVkStageFlags(range.stages);
		}
	}
	if(stages)
		vkCmd.pushConstants(vkLayout, stages, offset, static_cast<uint32_t>(size), data);
}

//-----------------

void CommandBuffer::bindVertexBuffers(uint32_t firstBinding, const std::vector<BufferObjectRef>& buffers, const std::vector<size_t>& offsets) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording. Call begin() first.",);
	vk::CommandBuffer vkCmd(handle);
	std::vector<vk::Buffer> vkBuffers;
	std::vector<vk::DeviceSize> vkOffsets(offsets.begin(), offsets.end());
	vkOffsets.resize(buffers.size(), 0);
	for(auto& bo : buffers) {
		vkBuffers.emplace_back((bo && bo->isValid()) ? bo->getApiHandle() : nullptr);
		boundBuffers.emplace_back(bo);
	}
	vkCmd.bindVertexBuffers(firstBinding, vkBuffers, vkOffsets);
}

//-----------------

void CommandBuffer::bindIndexBuffer(const BufferObjectRef& buffer, size_t offset) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording. Call begin() first.",);
	vk::CommandBuffer vkCmd(handle);
	vk::Buffer vkBuffer((buffer && buffer->isValid()) ? buffer->getApiHandle() : nullptr);
	vkCmd.bindIndexBuffer(vkBuffer, offset, vk::IndexType::eUint32);
	boundBuffers.emplace_back(buffer);
}

//-----------------

void CommandBuffer::clear(bool clearColor, bool clearDepth, bool clearStencil, const Geometry::Rect_i& rect) {
	WARN_AND_RETURN_IF(!inRenderPass, "Command buffer is not in a render pass. Call beginRenderPass() first.",);
	WARN_AND_RETURN_IF(!activeFBO || !activeFBO->isValid(), "Cannot clear attachments. Invalid FBO.",);

	std::vector<vk::ClearAttachment> clearAttachments;
	if(clearColor) {
		for(uint32_t i=0; i<activeFBO->getColorAttachmentCount(); ++i) {
			auto c = i < clearColors.size() ? clearColors[i] : Util::Color4f(0,0,0,0);
			vk::ClearAttachment att{};
			att.clearValue.color.setFloat32({c.r(), c.g(), c.b(), c.a()});
			att.colorAttachment = i;
			att.aspectMask = vk::ImageAspectFlagBits::eColor;
			clearAttachments.emplace_back(att);
		}
	}

	if(clearDepth || clearStencil) {
		vk::ClearAttachment att{};
		if(clearDepth)
			att.aspectMask |= vk::ImageAspectFlagBits::eDepth;
		if(clearStencil)
			att.aspectMask |= vk::ImageAspectFlagBits::eStencil;
		att.clearValue.depthStencil.depth = clearDepthValue;
		att.clearValue.depthStencil.stencil = clearStencilValue;
		clearAttachments.emplace_back(att);
	}

	vk::ClearRect clearRect;	
	clearRect.baseArrayLayer = 0;
	clearRect.layerCount = 1;
	clearRect.rect = vk::Rect2D{
		{rect.getX(), rect.getY()},
		{
			rect.getWidth() > 0 ? rect.getWidth() : activeFBO->getWidth(),
			rect.getHeight() > 0 ? rect.getHeight() : activeFBO->getHeight(),
		}
	};
	vk::CommandBuffer vkCmd(handle);
	vkCmd.clearAttachments(clearAttachments, {clearRect});
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
	WARN_AND_RETURN_IF(!texture, "Cannot clear image. Invalid texture.",);
	clearImage(texture->getImageView(), color);
}

//-----------------

void CommandBuffer::clearImage(const ImageViewRef& view, const Util::Color4f& color) {
	WARN_AND_RETURN_IF(!view, "Cannot clear image. Invalid image.",);
	vk::CommandBuffer vkCmd(handle);
	auto image = view->getImage();
	auto format = image->getFormat();
	vk::ImageSubresourceRange range = {};
	range.baseMipLevel = view->getMipLevel();
	range.levelCount = view->getMipLevelCount();
	range.baseArrayLayer = view->getLayer();
	range.layerCount = view->getLayerCount();
	
	imageBarrier(view, ResourceUsage::CopyDestination);
	if(isDepthStencilFormat(image->getFormat())) {
		vk::ClearDepthStencilValue clearValue{};
		clearValue.depth = color.r();
		clearValue.stencil = static_cast<uint32_t>(color.g());
		range.aspectMask = vk::ImageAspectFlagBits::eDepth |  vk::ImageAspectFlagBits::eStencil;
		vkCmd.clearDepthStencilImage(static_cast<vk::Image>(image->getApiHandle()), getVkImageLayout(image->getLastUsage()), clearValue, {range});
	} else {
		vk::ClearColorValue clearValue{};
		clearValue.setFloat32({color.r(), color.g(), color.b(), color.a()});
		range.aspectMask = vk::ImageAspectFlagBits::eColor;
		vkCmd.clearColorImage(static_cast<vk::Image>(image->getApiHandle()), getVkImageLayout(image->getLastUsage()), clearValue, {range});
	}
	boundResource.emplace_back(view->getApiHandle());
}

//-----------------

void CommandBuffer::clearImage(const ImageStorageRef& image, const Util::Color4f& color) {
	WARN_AND_RETURN_IF(!image, "Cannot clear image. Invalid image.",);
	vk::CommandBuffer vkCmd(handle);
	auto format = image->getFormat();
	vk::ImageSubresourceRange range = {};
	range.levelCount = format.mipLevels;
	range.layerCount = format.layers;
	
	imageBarrier(image, ResourceUsage::CopyDestination);
	if(isDepthStencilFormat(image->getFormat())) {
		vk::ClearDepthStencilValue clearValue{};
		clearValue.depth = color.r();
		clearValue.stencil = static_cast<uint32_t>(color.g());
		range.aspectMask = vk::ImageAspectFlagBits::eDepth |  vk::ImageAspectFlagBits::eStencil;
		vkCmd.clearDepthStencilImage(static_cast<vk::Image>(image->getApiHandle()), getVkImageLayout(image->getLastUsage()), clearValue, {range});
	} else {
		vk::ClearColorValue clearValue{};
		clearValue.setFloat32({color.r(), color.g(), color.b(), color.a()});
		range.aspectMask = vk::ImageAspectFlagBits::eColor;
		vkCmd.clearColorImage(static_cast<vk::Image>(image->getApiHandle()), getVkImageLayout(image->getLastUsage()), clearValue, {range});
	}
	boundResource.emplace_back(image->getApiHandle());
}

//-----------------

void CommandBuffer::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
	WARN_AND_RETURN_IF(!inRenderPass, "Command buffer is not in a render pass. Call beginRenderPass() first.",);
	if(instanceCount==0) return;
	vk::CommandBuffer vkCmd(handle);
	pipeline.setType(PipelineType::Graphics); // ensure we have a graphics pipeline
	flush();
	vkCmd.draw(vertexCount, instanceCount, firstVertex, firstInstance);
}

//-----------------

void CommandBuffer::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance) {
	WARN_AND_RETURN_IF(!inRenderPass, "Command buffer is not in a render pass. Call beginRenderPass() first.",);
	if(instanceCount==0) return;
	vk::CommandBuffer vkCmd(handle);
	pipeline.setType(PipelineType::Graphics); // ensure we have a graphics pipeline
	flush();
	vkCmd.drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

//-----------------

void CommandBuffer::drawIndirect(const BufferObjectRef& buffer, uint32_t drawCount, uint32_t stride, size_t offset) {
	WARN_AND_RETURN_IF(!inRenderPass, "Command buffer is not in a render pass. Call beginRenderPass() first.",);
	WARN_AND_RETURN_IF(!buffer->isValid(), "Cannot perform indirect draw. Buffer is not valid.",);
	vk::CommandBuffer vkCmd(handle);
	pipeline.setType(PipelineType::Graphics); // ensure we have a graphics pipeline
	flush();
	vk::Buffer vkBuffer(buffer->getApiHandle());
	vkCmd.drawIndirect(vkBuffer, offset, drawCount, stride);
}

//-----------------

void CommandBuffer::drawIndexedIndirect(const BufferObjectRef& buffer, uint32_t drawCount, uint32_t stride, size_t offset) {
	WARN_AND_RETURN_IF(!inRenderPass, "Command buffer is not in a render pass. Call beginRenderPass() first.",);
	WARN_AND_RETURN_IF(!buffer->isValid(), "Cannot perform indirect draw. Buffer is not valid.",);
	vk::CommandBuffer vkCmd(handle);
	pipeline.setType(PipelineType::Graphics); // ensure we have a graphics pipeline
	flush();
	vk::Buffer vkBuffer(buffer->getApiHandle());
	vkCmd.drawIndexedIndirect(vkBuffer, offset, drawCount, stride);
}

//-----------------


void CommandBuffer::copyBuffer(const BufferStorageRef& srcBuffer, const BufferStorageRef& tgtBuffer, size_t size, size_t srcOffset, size_t tgtOffset) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording. Call begin() first.",);
	WARN_AND_RETURN_IF(!srcBuffer || !tgtBuffer, "Cannot copy buffer. Invalid buffers.",);
	vk::CommandBuffer vkCmd(handle);
	vkCmd.copyBuffer(static_cast<vk::Buffer>(srcBuffer->getApiHandle()), static_cast<vk::Buffer>(tgtBuffer->getApiHandle()), {{srcOffset,tgtOffset,size}});
	boundResource.emplace_back(srcBuffer->getApiHandle());
	boundResource.emplace_back(tgtBuffer->getApiHandle());
}

//-----------------

void CommandBuffer::copyBuffer(const BufferObjectRef& srcBuffer, const BufferObjectRef& tgtBuffer, size_t size, size_t srcOffset, size_t tgtOffset) {
	if(srcBuffer && tgtBuffer)
		copyBuffer(srcBuffer->getBuffer(), tgtBuffer->getBuffer(), size, srcOffset, tgtOffset);
	boundBuffers.emplace_back(srcBuffer);
	boundBuffers.emplace_back(tgtBuffer);
}

//-----------------

void CommandBuffer::updateBuffer(const BufferStorageRef& buffer, const uint8_t* data, size_t size, size_t offset) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording. Call begin() first.",);
	WARN_AND_RETURN_IF(!buffer || !data, "Cannot update buffer. Invalid buffer or data.",);
	WARN_AND_RETURN_IF(size+offset > buffer->getSize(), "Cannot update buffer. Offset+size exceeds buffer size.",);
	vk::CommandBuffer vkCmd(handle);
	vkCmd.updateBuffer(static_cast<vk::Buffer>(buffer->getApiHandle()), offset, size, data);
	boundResource.emplace_back(buffer->getApiHandle());
}

//-----------------

void CommandBuffer::copyImage(const ImageStorageRef& srcImage, const ImageStorageRef& tgtImage, const ImageRegion& srcRegion, const ImageRegion& tgtRegion) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording. Call begin() first.",);
	WARN_AND_RETURN_IF(!srcImage || !tgtImage, "Cannot copy image. Invalid images.",);
	WARN_AND_RETURN_IF(srcRegion.extent != tgtRegion.extent, "Cannot copy image. Source and target extent must be the same.",);

	vk::CommandBuffer vkCmd(handle);
	imageBarrier(srcImage, ResourceUsage::CopySource);
	imageBarrier(tgtImage, ResourceUsage::CopyDestination);
	vk::ImageAspectFlags srcAspect = isDepthStencilFormat(srcImage->getFormat()) ? (vk::ImageAspectFlagBits::eDepth |  vk::ImageAspectFlagBits::eStencil) : vk::ImageAspectFlagBits::eColor;
	vk::ImageAspectFlags tgtAspect = isDepthStencilFormat(tgtImage->getFormat()) ? (vk::ImageAspectFlagBits::eDepth |  vk::ImageAspectFlagBits::eStencil) : vk::ImageAspectFlagBits::eColor;
	vk::ImageCopy copyRegion{
		{srcAspect, srcRegion.mipLevel, srcRegion.baseLayer, srcRegion.layerCount}, {srcRegion.offset.x(), srcRegion.offset.y(), srcRegion.offset.z()},
		{tgtAspect, tgtRegion.mipLevel, tgtRegion.baseLayer, tgtRegion.layerCount}, {tgtRegion.offset.x(), tgtRegion.offset.y(), tgtRegion.offset.z()},
		{srcRegion.extent.x(), srcRegion.extent.y(), srcRegion.extent.z()}
	};
	vkCmd.copyImage(
		static_cast<vk::Image>(srcImage->getApiHandle()), getVkImageLayout(srcImage->getLastUsage()),
		static_cast<vk::Image>(tgtImage->getApiHandle()), getVkImageLayout(tgtImage->getLastUsage()),
		{copyRegion}
	);
	boundResource.emplace_back(srcImage->getApiHandle());
	boundResource.emplace_back(tgtImage->getApiHandle());
}

//-----------------

void CommandBuffer::copyBufferToImage(const BufferStorageRef& srcBuffer, const ImageStorageRef& tgtImage, size_t srcOffset, const ImageRegion& tgtRegion) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording. Call begin() first.",);
	WARN_AND_RETURN_IF(!srcBuffer || !tgtImage, "Cannot copy buffer to image. Invalid buffer or image.",);

	vk::CommandBuffer vkCmd(handle);
	imageBarrier(tgtImage, ResourceUsage::CopyDestination);
	vk::ImageAspectFlags tgtAspect = isDepthStencilFormat(tgtImage->getFormat()) ? (vk::ImageAspectFlagBits::eDepth |  vk::ImageAspectFlagBits::eStencil) : vk::ImageAspectFlagBits::eColor;
	vk::BufferImageCopy copyRegion{
		static_cast<vk::DeviceSize>(srcOffset), 0u, 0u,
		{tgtAspect, tgtRegion.mipLevel, tgtRegion.baseLayer, tgtRegion.layerCount}, {tgtRegion.offset.x(), tgtRegion.offset.y(), tgtRegion.offset.z()},
		{tgtRegion.extent.x(), tgtRegion.extent.y(), tgtRegion.extent.z()}
	};
	vkCmd.copyBufferToImage(
		static_cast<vk::Buffer>(srcBuffer->getApiHandle()),
		static_cast<vk::Image>(tgtImage->getApiHandle()), getVkImageLayout(tgtImage->getLastUsage()),
		{copyRegion}
	);
	boundResource.emplace_back(srcBuffer->getApiHandle());
	boundResource.emplace_back(tgtImage->getApiHandle());
}

//-----------------

void CommandBuffer::copyImageToBuffer(const ImageStorageRef& srcImage, const BufferStorageRef& tgtBuffer, const ImageRegion& srcRegion, size_t tgtOffset) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording. Call begin() first.",);
	WARN_AND_RETURN_IF(!srcImage || !tgtBuffer, "Cannot copy image to buffer. Invalid buffer or image.",);

	vk::CommandBuffer vkCmd(handle);	
	imageBarrier(srcImage, ResourceUsage::CopySource);
	vk::ImageAspectFlags srcAspect = isDepthStencilFormat(srcImage->getFormat()) ? (vk::ImageAspectFlagBits::eDepth |  vk::ImageAspectFlagBits::eStencil) : vk::ImageAspectFlagBits::eColor;
	vk::BufferImageCopy copyRegion{
		static_cast<vk::DeviceSize>(tgtOffset), 0u, 0u,
		{srcAspect, srcRegion.mipLevel, srcRegion.baseLayer, srcRegion.layerCount}, {srcRegion.offset.x(), srcRegion.offset.y(), srcRegion.offset.z()},
		{srcRegion.extent.x(), srcRegion.extent.y(), srcRegion.extent.z()}
	};
	vkCmd.copyImageToBuffer(
		static_cast<vk::Image>(srcImage->getApiHandle()), getVkImageLayout(srcImage->getLastUsage()),
		static_cast<vk::Buffer>(tgtBuffer->getApiHandle()),
		{copyRegion}
	);
	boundResource.emplace_back(srcImage->getApiHandle());
	boundResource.emplace_back(tgtBuffer->getApiHandle());
}

//-----------------

void CommandBuffer::blitImage(const ImageStorageRef& srcImage, const ImageStorageRef& tgtImage, const ImageRegion& srcRegion, const ImageRegion& tgtRegion, ImageFilter filter) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording. Call begin() first.",);
	WARN_AND_RETURN_IF(!srcImage || !tgtImage, "Cannot blit image. Invalid images.",);

	vk::CommandBuffer vkCmd(handle);
	imageBarrier(srcImage, ResourceUsage::CopySource);
	imageBarrier(tgtImage, ResourceUsage::CopyDestination);
	vk::ImageAspectFlags srcAspect = isDepthStencilFormat(srcImage->getFormat()) ? (vk::ImageAspectFlagBits::eDepth |  vk::ImageAspectFlagBits::eStencil) : vk::ImageAspectFlagBits::eColor;
	vk::ImageAspectFlags tgtAspect = isDepthStencilFormat(tgtImage->getFormat()) ? (vk::ImageAspectFlagBits::eDepth |  vk::ImageAspectFlagBits::eStencil) : vk::ImageAspectFlagBits::eColor;
	Geometry::Vec3i srcOffset2 = srcRegion.offset + toVec3i(srcRegion.extent);
	Geometry::Vec3i tgtOffset2 = srcRegion.offset + toVec3i(srcRegion.extent);
	vk::ImageBlit blitRegion{
		{srcAspect, srcRegion.mipLevel, srcRegion.baseLayer, srcRegion.layerCount},
		{vk::Offset3D{srcRegion.offset.x(), srcRegion.offset.y(), srcRegion.offset.z()}, vk::Offset3D{srcOffset2.x(), srcOffset2.y(), srcOffset2.z()}},
		{tgtAspect, tgtRegion.mipLevel, tgtRegion.baseLayer, tgtRegion.layerCount},
		{vk::Offset3D{tgtRegion.offset.x(), tgtRegion.offset.y(), tgtRegion.offset.z()}, vk::Offset3D{tgtOffset2.x(), tgtOffset2.y(), tgtOffset2.z()}},
	};
	vkCmd.blitImage(
		static_cast<vk::Image>(srcImage->getApiHandle()), getVkImageLayout(srcImage->getLastUsage()),
		static_cast<vk::Image>(tgtImage->getApiHandle()), getVkImageLayout(tgtImage->getLastUsage()),
		{blitRegion}, getVkFilter(filter)
	);
	boundResource.emplace_back(srcImage->getApiHandle());
	boundResource.emplace_back(tgtImage->getApiHandle());
}

//-----------------

void CommandBuffer::imageBarrier(const TextureRef& texture, ResourceUsage newUsage) {
	WARN_AND_RETURN_IF(!texture, "Cannot create image barrier. Invalid texture.",);
	imageBarrier(texture->getImageView(), newUsage);
}

//-----------------

void CommandBuffer::imageBarrier(const ImageViewRef& view, ResourceUsage newUsage) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording. Call begin() first.",);
	WARN_AND_RETURN_IF(!view, "Cannot create image barrier. Invalid image.",);
	auto image = view->getImage();
	if(view->getLastUsage() == newUsage || view->getLastUsage() == ResourceUsage::General)
		return;

	vk::CommandBuffer vkCmd(handle);
	const auto& format = image->getFormat();

	vk::ImageMemoryBarrier barrier{};
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.srcAccessMask = getVkAccessMask(view->getLastUsage());
	barrier.dstAccessMask = getVkAccessMask(newUsage);
	barrier.oldLayout = getVkImageLayout(view->getLastUsage());
	barrier.newLayout = getVkImageLayout(newUsage);
	barrier.image = image->getApiHandle();
	barrier.subresourceRange = {
		isDepthStencilFormat(format) ? (vk::ImageAspectFlagBits::eDepth |  vk::ImageAspectFlagBits::eStencil) : vk::ImageAspectFlagBits::eColor,
		view->getMipLevel(), view->getMipLevelCount(),
		view->getLayer(), view->getLayerCount()
	};

	vkCmd.pipelineBarrier(
		getVkPipelineStageMask(view->getLastUsage(), true),
		getVkPipelineStageMask(newUsage, false),
		{}, {}, {}, {barrier}
	);
	view->_setLastUsage(newUsage);
}

//-----------------

void CommandBuffer::imageBarrier(const ImageStorageRef& image, ResourceUsage newUsage) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording. Call begin() first.",);
	WARN_AND_RETURN_IF(!image, "Cannot create image barrier. Invalid image.",);
	if(image->getLastUsage() == newUsage || image->getLastUsage() == ResourceUsage::General)
		return;

	vk::CommandBuffer vkCmd(handle);
	const auto& format = image->getFormat();	

	vk::ImageMemoryBarrier barrier{};
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.srcAccessMask = getVkAccessMask(image->getLastUsage());
	barrier.dstAccessMask = getVkAccessMask(newUsage);
	barrier.oldLayout = getVkImageLayout(image->getLastUsage());
	barrier.newLayout = getVkImageLayout(newUsage);
	barrier.image = image->getApiHandle();
	barrier.subresourceRange = { 
		isDepthStencilFormat(format) ? (vk::ImageAspectFlagBits::eDepth |  vk::ImageAspectFlagBits::eStencil) : vk::ImageAspectFlagBits::eColor,
		0, format.mipLevels,
		0, format.layers
	};

	vkCmd.pipelineBarrier(
		getVkPipelineStageMask(image->getLastUsage(), true),
		getVkPipelineStageMask(newUsage, false),
		{}, {}, {}, {barrier}
	);
	image->_setLastUsage(newUsage);
}

//-----------------

/*void CommandBuffer::bufferBarrier(const BufferObjectRef& buffer, ResourceUsage newUsage) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording. Call begin() first.",);
	vk::CommandBuffer vkCmd(handle);
}*/

//-----------------

void CommandBuffer::beginDebugMarker(const std::string& name, const Util::Color4f& color) {
	if(!queue->getDevice()->getConfig().debugMode)
		return;
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording. Call begin() first.",);	
	vk::CommandBuffer vkCmd(handle);
	vkCmd.beginDebugUtilsLabelEXT({name.c_str(), {color.r(), color.g(), color.b(), color.a()}});
}

//-----------------

void CommandBuffer::insertDebugMarker(const std::string& name, const Util::Color4f& color) {
	if(!queue->getDevice()->getConfig().debugMode)
		return;
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording. Call begin() first.",);
	vk::CommandBuffer vkCmd(handle);
	vkCmd.insertDebugUtilsLabelEXT({name.c_str(), {color.r(), color.g(), color.b(), color.a()}});
}

//-----------------

void CommandBuffer::endDebugMarker() {
	if(!queue->getDevice()->getConfig().debugMode)
		return;
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording. Call begin() first.",);
	vk::CommandBuffer vkCmd(handle);
	vkCmd.endDebugUtilsLabelEXT();
}

//-------------

void CommandBuffer::setDebugName(const std::string& name) {
	if(!queue->getDevice()->getConfig().debugMode)
		return;
	vk::Device vkDevice(queue->getDevice()->getApiHandle());
	vkDevice.setDebugUtilsObjectNameEXT({ vk::CommandBuffer::objectType, handle, name.c_str() });
}

//-----------------
} // namespace Rendering

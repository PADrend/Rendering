/*
	This file is part of the Rendering library.
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "CommandBuffer.h"
#include "Device.h"
#include "DescriptorSet.h"
#include "DescriptorPool.h"
#include "Pipeline.h"
#include "ImageStorage.h"
#include "ImageView.h"
#include "BufferStorage.h"
#include "../Shader/Shader.h"
#include "../Texture/Texture.h"
#include "../BufferObject.h"
#include "../FBO.h"

#include <Util/Macros.h>

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

CommandBuffer::Ref CommandBuffer::create(const DeviceRef& device, QueueFamily family, bool primary) {
	return create(device->getQueue(family));
}

//-----------------

CommandBuffer::Ref CommandBuffer::create(const QueueRef& queue, bool primary) {
	auto buffer = new CommandBuffer(queue, primary);
	if(!buffer->init())
		return nullptr;
	return buffer;
}

//-----------------

CommandBuffer::CommandBuffer(const QueueRef& queue, bool primary) : queue(queue), primary(primary) {
	pipeline = Pipeline::createGraphics(queue->getDevice());
}

//-----------------

CommandBuffer::~CommandBuffer() {
	if(handle)
		queue->freeCommandBuffer(handle, primary);
};

//-----------------

bool CommandBuffer::init() {
	handle = queue->requestCommandBuffer(primary);
	if(handle)
		state = Initial;
	return handle.isNotNull();
}

//-----------------

void CommandBuffer::reset() {
	vk::CommandBuffer vkCmd(handle);
	if(state == State::Recording)
		end();
	vkCmd.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
	pipeline = Pipeline::createGraphics(queue->getDevice());
	descriptorSets.clear();
	boundPipeline = nullptr;
	state = State::Initial;
}

//-----------------

void CommandBuffer::flush() {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording. Call begin() first.",);
	vk::CommandBuffer vkCmd(handle);
	auto device = queue->getDevice();
	auto shader = pipeline->getShader();
	auto layout = shader->getLayout();

	// bind pipeline if it has changed
	WARN_AND_RETURN_IF(!pipeline->validate(), "CommandBuffer: Invalid Pipeline.",);

	vk::Pipeline vkPipeline(pipeline->getApiHandle());
	vk::PipelineLayout vkPipelineLayout(shader->getLayoutHandle());
	vk::PipelineBindPoint vkBindPoint;
	switch (pipeline->getType()) {
		case PipelineType::Compute:
			vkBindPoint = vk::PipelineBindPoint::eCompute;
			break;
		case PipelineType::Graphics:
			vkBindPoint = vk::PipelineBindPoint::eGraphics;
			break;
		default:
			break;
	}

	if(!boundPipeline || boundPipeline != pipeline->getApiHandle()) {
		vkCmd.bindPipeline(vkBindPoint, vkPipeline);
		boundPipeline = pipeline->getApiHandle();
	}

	// remove unused descriptor sets
	for(auto it = descriptorSets.begin(); it != descriptorSets.end();) {
		if(!layout.hasLayoutSet(it->first)) {
			it = descriptorSets.erase(it);
		} else {
			++it;
		}
	}

	// bindings did not change
	if(!bindings.isDirty())
		return;
	bindings.clearDirty();

	for(auto& it : bindings.getBindingSets()) {
		auto set = it.first;
		auto& bindingSet = it.second;
		if(!bindingSet.isDirty())
			continue;
		bindings.clearDirty(set);
		if(!layout.hasLayoutSet(set))
			continue;
		
		DescriptorSet::Ref descriptorSet;
		auto dIt = descriptorSets.find(set);
		auto pool = shader->getDescriptorPool(set);
		if(dIt == descriptorSets.end() || dIt->second->getLayoutHandle() != pool->getLayoutHandle()) {
			descriptorSet = DescriptorSet::create(pool);
			descriptorSets.emplace(set, descriptorSet);
		} else {
			descriptorSet = dIt->second;
		}

		descriptorSet->update(bindingSet);
		vk::DescriptorSet vkDescriptorSet(descriptorSet->getApiHandle());
		vkCmd.bindDescriptorSets(vkBindPoint, vkPipelineLayout, set, {vkDescriptorSet}, descriptorSet->getDynamicOffsets());
	}

}

//-----------------

void CommandBuffer::begin() {
	WARN_AND_RETURN_IF(state == State::Recording, "Command buffer is already recording.",);
	WARN_AND_RETURN_IF(state == State::Invalid, "Invalid command buffer.",);
	vk::CommandBuffer vkCmd(handle);
	state = State::Recording;
	vkCmd.begin({vk::CommandBufferUsageFlagBits::eSimultaneousUse});
}

//-----------------

void CommandBuffer::end() {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording. Call begin() first.",);
	vk::CommandBuffer vkCmd(handle);
	state = State::Executable;
	vkCmd.end();
}

//-----------------

void CommandBuffer::submit(bool wait) {
	WARN_AND_RETURN_IF(isRecording(), "Command buffer is currently recording. Call end() first.",);
	vk::CommandBuffer vkCmd(handle);
	queue->submit(this, wait);
}

//-----------------

void CommandBuffer::beginRenderPass(const std::vector<Util::Color4f>& clearColors) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording. Call begin() first.",);
	WARN_AND_RETURN_IF(inRenderPass, "Command buffer is already in a render pass. Call endRenderPass() first.",);
	vk::CommandBuffer vkCmd(handle);
	auto& fbo = getFBO();
	WARN_AND_RETURN_IF(!fbo || !fbo->validate(), "Cannot begin render pass. Invalid FBO.",);

	vk::Framebuffer framebuffer(fbo->getApiHandle());
	vk::RenderPass renderPass(fbo->getRenderPass());

	std::vector<vk::ClearValue> clearValues(fbo->getColorAttachmentCount(), vk::ClearColorValue{});
	for(uint32_t i=0; i<std::min<size_t>(clearValues.size(), clearColors.size()); ++i) {
		auto& c = clearColors[i];
		clearValues[i].color.setFloat32({c.r(), c.g(), c.b(), c.a()});
	}

	vkCmd.beginRenderPass({
		renderPass, framebuffer,
		vk::Rect2D{ {0, 0}, {fbo->getWidth(), fbo->getHeight()} },
		static_cast<uint32_t>(clearValues.size()), clearValues.data()
	}, vk::SubpassContents::eInline);
	inRenderPass = true;
}

//-----------------

void CommandBuffer::endRenderPass() {
	WARN_AND_RETURN_IF(!inRenderPass, "Command buffer is not in a render pass. Call beginRenderPass() first.",);
	vk::CommandBuffer vkCmd(handle);
	vkCmd.endRenderPass();
	inRenderPass = false;
}

//-----------------

void CommandBuffer::bindBuffer(const BufferObjectRef& buffer, uint32_t set, uint32_t binding, uint32_t arrayElement) {
	bindings.bindBuffer(buffer, set, binding, arrayElement);
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

void CommandBuffer::pushConstants(const std::vector<uint8_t>& data, uint32_t offset) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording. Call begin() first.",);
	vk::CommandBuffer vkCmd(handle);
	Shader::Ref shader = pipeline->getShader();
	WARN_AND_RETURN_IF(!shader || !shader->init(), "Cannot set push constants. No bound shader.",);
	auto& layout = shader->getLayout();
	vk::PipelineLayout vkLayout(shader->getLayoutHandle());
	vk::ShaderStageFlags stages{};
	for(auto& range : layout.getPushConstantRanges()) {
		if(offset >= range.offset && offset + data.size() <= range.offset + range.size) {
			stages |= getVkStageFlags(range.stages);
		}
	}
	if(stages)
		vkCmd.pushConstants(vkLayout, stages, offset, static_cast<uint32_t>(data.size()), data.data());
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
	}
	vkCmd.bindVertexBuffers(firstBinding, vkBuffers, vkOffsets);
}

//-----------------

void CommandBuffer::bindIndexBuffer(const BufferObjectRef& buffer, size_t offset) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording. Call begin() first.",);
	vk::CommandBuffer vkCmd(handle);
	vk::Buffer vkBuffer((buffer && buffer->isValid()) ? buffer->getApiHandle() : nullptr);
	vkCmd.bindIndexBuffer(vkBuffer, offset, vk::IndexType::eUint32);
}

//-----------------

void CommandBuffer::clearColor(const std::vector<Util::Color4f>& clearColors, const Geometry::Rect_i& rect) {
	WARN_AND_RETURN_IF(!inRenderPass, "Command buffer is not in a render pass. Call beginRenderPass() first.",);
	auto& fbo = getFBO();
	WARN_AND_RETURN_IF(!fbo || !fbo->isValid(), "Cannot clear color. Invalid FBO.",);
	std::vector<vk::ClearAttachment> clearAttachments(clearColors.size(), vk::ClearAttachment{});
	for(uint32_t i=0; i<std::min<size_t>(clearAttachments.size(), clearColors.size()); ++i) {
		auto& c = clearColors[i];
		clearAttachments[i].clearValue.color.setFloat32({c.r(), c.g(), c.b(), c.a()});
		clearAttachments[i].colorAttachment = i;
		clearAttachments[i].aspectMask = vk::ImageAspectFlagBits::eColor;
	}
	vk::ClearRect clearRect;
	
	clearRect.baseArrayLayer = 0;
	clearRect.layerCount = 1;
	clearRect.rect = vk::Rect2D{
		{rect.getX(), rect.getY()},
		{
			rect.getWidth() > 0 ? rect.getWidth() : fbo->getWidth(),
			rect.getHeight() > 0 ? rect.getHeight() : fbo->getHeight(),
		}
	};
	vk::CommandBuffer vkCmd(handle);
	vkCmd.clearAttachments(clearAttachments, {clearRect});
}

//-----------------

void CommandBuffer::clearDepthStencil(float depth, uint32_t stencil, const Geometry::Rect_i& rect, bool clearDepth, bool clearStencil) {
	WARN_AND_RETURN_IF(!inRenderPass, "Command buffer is not in a render pass. Call beginRenderPass() first.",);
	auto& fbo = getFBO();
	WARN_AND_RETURN_IF(!fbo || !fbo->isValid(), "Cannot clear depth stencil. Invalid FBO.",);
	vk::ClearAttachment clearAttachment;
	if(clearDepth)
		clearAttachment.aspectMask |= vk::ImageAspectFlagBits::eDepth;
	if(clearStencil)
		clearAttachment.aspectMask |= vk::ImageAspectFlagBits::eStencil;
	clearAttachment.clearValue.depthStencil.depth = depth;
	clearAttachment.clearValue.depthStencil.stencil = stencil;
	vk::ClearRect clearRect;
	clearRect.baseArrayLayer = 0;
	clearRect.layerCount = 1;
	clearRect.rect = vk::Rect2D{
		{rect.getX(), rect.getY()},
		{
			rect.getWidth() > 0 ? rect.getWidth() : fbo->getWidth(),
			rect.getHeight() > 0 ? rect.getHeight() : fbo->getHeight(),
		}
	};
	vk::CommandBuffer vkCmd(handle);
	vkCmd.clearAttachments({clearAttachment}, {clearRect});
}

//-----------------

void CommandBuffer::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
	WARN_AND_RETURN_IF(!inRenderPass, "Command buffer is not in a render pass. Call beginRenderPass() first.",);
	if(instanceCount==0) return;
	vk::CommandBuffer vkCmd(handle);
	pipeline->setType(PipelineType::Graphics); // ensure we have a graphics pipeline
	flush();
	vkCmd.draw(vertexCount, instanceCount, firstVertex, firstInstance);
}

//-----------------

void CommandBuffer::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance) {
	WARN_AND_RETURN_IF(!inRenderPass, "Command buffer is not in a render pass. Call beginRenderPass() first.",);
	if(instanceCount==0) return;
	vk::CommandBuffer vkCmd(handle);
	pipeline->setType(PipelineType::Graphics); // ensure we have a graphics pipeline
	flush();
	vkCmd.drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

//-----------------

void CommandBuffer::drawIndirect(const BufferObjectRef& buffer, uint32_t drawCount, uint32_t stride, size_t offset) {
	WARN_AND_RETURN_IF(!inRenderPass, "Command buffer is not in a render pass. Call beginRenderPass() first.",);
	WARN_AND_RETURN_IF(!buffer->isValid(), "Cannot perform indirect draw. Buffer is not valid.",);
	vk::CommandBuffer vkCmd(handle);
	pipeline->setType(PipelineType::Graphics); // ensure we have a graphics pipeline
	flush();
	vk::Buffer vkBuffer(buffer->getApiHandle());
	vkCmd.drawIndirect(vkBuffer, offset, drawCount, stride);
}

//-----------------

void CommandBuffer::drawIndexedIndirect(const BufferObjectRef& buffer, uint32_t drawCount, uint32_t stride, size_t offset) {
	WARN_AND_RETURN_IF(!inRenderPass, "Command buffer is not in a render pass. Call beginRenderPass() first.",);
	WARN_AND_RETURN_IF(!buffer->isValid(), "Cannot perform indirect draw. Buffer is not valid.",);
	vk::CommandBuffer vkCmd(handle);
	pipeline->setType(PipelineType::Graphics); // ensure we have a graphics pipeline
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
}

//-----------------

void CommandBuffer::copyBuffer(const BufferObjectRef& srcBuffer, const BufferObjectRef& tgtBuffer, size_t size, size_t srcOffset, size_t tgtOffset) {
	if(srcBuffer && tgtBuffer)
		copyBuffer(srcBuffer->getBuffer(), tgtBuffer->getBuffer(), size, srcOffset, tgtOffset);
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
}

//-----------------

void CommandBuffer::textureBarrier(const TextureRef& texture, ResourceUsage newUsage) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording. Call begin() first.",);
	WARN_AND_RETURN_IF(!texture || !texture->isValid(), "Cannot create texture barrier. Invalid texture.",);
	auto view = texture->getImageView();
	auto image = texture->getImage();
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

} // namespace Rendering

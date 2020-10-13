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

namespace Rendering {

//-----------------

vk::AccessFlags getVkAccessMask(const ResourceUsage& usage);
vk::ImageLayout getVkImageLayout(const ResourceUsage& usage);
vk::PipelineStageFlags getVkPipelineStageMask(const ResourceUsage& usage, bool src);

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
	vk::CommandBuffer vkCmd(handle);
	auto& fbo = getFBO();
	WARN_AND_RETURN_IF(!fbo || !fbo->validate(), "Cannot begin render pass. Invalid FBO.",);

	vk::Framebuffer framebuffer(fbo->getApiHandle());
	vk::RenderPass renderPass(fbo->getRenderPass());

	std::vector<vk::ClearValue> clearValues(fbo->getColorAttachmentCount(), vk::ClearColorValue{});
	for(uint32_t i=0; i<clearValues.size(); ++i) {
		auto& c = clearColors[i];
		clearValues[i].color.setFloat32(std::array<float,4>{c.r(), c.g(), c.b(), c.a()});
	}

	vkCmd.beginRenderPass({
		renderPass, framebuffer,
		vk::Rect2D{ {0, 0}, {fbo->getWidth(), fbo->getHeight()} },
		static_cast<uint32_t>(clearValues.size()), clearValues.data()
	}, vk::SubpassContents::eInline);
}

//-----------------

void CommandBuffer::endRenderPass() {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording. Call begin() first.",);
	vk::CommandBuffer vkCmd(handle);
	vkCmd.endRenderPass();
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

void CommandBuffer::bindVertexBuffers(uint32_t firstBinding, const std::vector<BufferObjectRef>& buffers, const std::vector<size_t>& offsets) {
	vk::CommandBuffer vkCmd(handle);
	std::vector<vk::Buffer> vkBuffers;
	std::vector<vk::DeviceSize> vkOffsets(offsets.begin(), offsets.end());
	vkOffsets.resize(buffers.size(), 0);
	for(auto& bo : buffers) {
		vkBuffers.emplace_back((bo && bo->isValid()) ? bo->getBuffer()->getApiHandle() : nullptr);
	}
	vkCmd.bindVertexBuffers(firstBinding, vkBuffers, offsets);
}

//-----------------

void CommandBuffer::bindIndexBuffer(const BufferObjectRef& buffer, size_t offset) {
	vk::CommandBuffer vkCmd(handle);
	vk::Buffer vkBuffer((buffer && buffer->isValid()) ? buffer->getBuffer()->getApiHandle() : nullptr);
	vkCmd.bindIndexBuffer(vkBuffer, offset, vk::IndexType::eUint32);
}

//-----------------

void CommandBuffer::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording. Call begin() first.",);
	if(instanceCount==0) return;
	vk::CommandBuffer vkCmd(handle);
	pipeline->setType(PipelineType::Graphics); // ensure we have a graphics pipeline
	flush();
	vkCmd.draw(vertexCount, instanceCount, firstVertex, firstInstance);
}

//-----------------

void CommandBuffer::copyBuffer(const BufferStorageRef& srcBuffer, const BufferStorageRef& tgtBuffer, size_t size) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording. Call begin() first.",);
	WARN_AND_RETURN_IF(!srcBuffer || !tgtBuffer, "Cannot copy buffers. Invalid buffers.",);
	vk::CommandBuffer vkCmd(handle);
	vkCmd.copyBuffer(static_cast<vk::Buffer>(srcBuffer->getApiHandle()), static_cast<vk::Buffer>(tgtBuffer->getApiHandle()), {{0,0,size}});
}

//-----------------

void CommandBuffer::copyBuffer(const BufferObjectRef& srcBuffer, const BufferObjectRef& tgtBuffer, size_t size) {
	if(srcBuffer && tgtBuffer)
		copyBuffer(srcBuffer->getBuffer(), tgtBuffer->getBuffer(), size);
}

//-----------------

void CommandBuffer::textureBarrier(const TextureRef& texture, ResourceUsage newUsage) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording. Call begin() first.",);
	WARN_AND_RETURN_IF(!texture || !texture->isValid(), "Cannot create texture barrier. Invalid texture.",);
	auto view = texture->getImageView();
	auto image = texture->getImage();
	if(view->getLastUsage() == newUsage)
		return;

	vk::CommandBuffer vkCmd(handle);

	vk::ImageMemoryBarrier barrier{};
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.srcAccessMask = getVkAccessMask(view->getLastUsage());
	barrier.dstAccessMask = getVkAccessMask(newUsage);
	barrier.oldLayout = getVkImageLayout(view->getLastUsage());
	barrier.newLayout = getVkImageLayout(newUsage);
	barrier.image = image->getApiHandle();
	barrier.subresourceRange = { 
		vk::ImageAspectFlagBits::eColor, // TODO: check for depth/stencil format
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

/*void CommandBuffer::bufferBarrier(const BufferObjectRef& buffer, ResourceUsage newUsage) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording. Call begin() first.",);
	vk::CommandBuffer vkCmd(handle);
}*/

//-----------------

} // namespace Rendering

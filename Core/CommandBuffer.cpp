/*
	This file is part of the Rendering library.
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "CommandBuffer.h"
#include "CommandPool.h"
#include "Device.h"
#include "DescriptorSet.h"
#include "DescriptorPool.h"
#include "Pipeline.h"
#include "PipelineCache.h"
#include "ImageStorage.h"
#include "ImageView.h"
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

CommandBuffer::Ref CommandBuffer::request(const DeviceRef& device, QueueFamily family, bool primary) {
	auto pool = device->getCommandPool(family);
	if(!pool) 
		return nullptr;
	return pool->requestCommandBuffer(primary);
}

//-----------------

CommandBuffer::CommandBuffer(CommandPool* pool, bool primary) : pool(pool), primary(primary) { }

//-----------------

CommandBuffer::~CommandBuffer() = default;

//-----------------

bool CommandBuffer::init() {
	vk::Device vkDevice(pool->getApiHandle());
	vk::CommandPool vkPool(pool->getApiHandle());

	auto buffers = vkDevice.allocateCommandBuffers({
		vkPool,
		primary ? vk::CommandBufferLevel::ePrimary : vk::CommandBufferLevel::eSecondary,
		1u
	});

	if(buffers.empty() || !buffers.front())
		return false;

	handle = CommandBufferHandle::create(buffers.front(), {vkDevice, vkPool});
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
	pipelineState.reset();
	state = State::Initial;
}

//-----------------

void CommandBuffer::free() {
	vk::CommandBuffer vkCmd(handle);
	if(state == State::Recording)
		end();
	state = State::Free;
}


//-----------------

void CommandBuffer::flush(PipelineType bindPoint) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording. Call begin() first.",);
	vk::CommandBuffer vkCmd(handle);
	auto device = pool->getDevice();

	// bind pipeline if it has changed
	if(!pipeline || pipeline->getHash() != pipelineState.getHash()) {
		pipeline = device->getPipelineCache()->requestPipeline(bindPoint, pipelineState);
		WARN_AND_RETURN_IF(!pipeline, "CommandBuffer: Could not create pipeline.",);
		vk::Pipeline vkPipeline(pipeline->getApiHandle());	
		vk::PipelineBindPoint vkBindPoint;
		switch (bindPoint) {
			case PipelineType::Compute:
				vkBindPoint = vk::PipelineBindPoint::eCompute;
				break;
			case PipelineType::Graphics:
				vkBindPoint = vk::PipelineBindPoint::eGraphics;
				break;
			default:
				break;
		}
		vkCmd.bindPipeline(vkBindPoint, vkPipeline);
	}

	// bind descriptor sets
	if(!bindings.isDirty())
		return; // nothing has changed

	auto shader = pipelineState.getShader();
	WARN_AND_RETURN_IF(!shader || !shader->init(), "CommandBuffer: Could not bind descriptor sets. Invalid shader.",);

	std::unordered_set<uint32_t> updateSets;

	for(auto& poolIt : shader->getDescriptorPools()) {
		auto descrPool = poolIt.second;		
		auto it = bindings.getBindingSets().find(poolIt.first);
		// Check if set was bound before
		if(it != bindings.getBindingSets().end()) {
			// Check if layout changed
		}
	}
}

//-----------------

void CommandBuffer::begin() {
	WARN_AND_RETURN_IF(state == State::Recording, "Command buffer is already recording.",);
	WARN_AND_RETURN_IF(state == State::Free || state == State::Invalid, "Invalid command buffer.",);
	vk::CommandBuffer vkCmd(handle);
	state = State::Recording;
	pipelineState.reset();
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

void CommandBuffer::beginRenderPass(const std::vector<Util::Color4f>& clearColors) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording. Call begin() first.",);
	vk::CommandBuffer vkCmd(handle);
	auto& fbo = pipelineState.getFBO();
	WARN_AND_RETURN_IF(!fbo || !fbo->validate(), "Cannot begin render pass. Invalid FBO.",);
	vk::Framebuffer framebuffer(fbo->getApiHandle());
	vk::RenderPass renderPass(fbo->getRenderPass());
	bindings.reset();

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

void CommandBuffer::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording. Call begin() first.",);
	if(instanceCount==0) return;
	flush(PipelineType::Graphics);
	vk::CommandBuffer vkCmd(handle);
	vkCmd.draw(vertexCount, instanceCount, firstVertex, firstInstance);
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

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
#include "Pipeline.h"
#include "PipelineCache.h"
#include "ImageStorage.h"
#include "ImageView.h"
#include "../Shader/Shader.h"
#include "../Texture/Texture.h"
//#include "../BufferObject.h"
#include "../FBO.h"

#include <Util/Macros.h>

#include <vulkan/vulkan.hpp>

namespace Rendering {

//-----------------

vk::AccessFlags getAccessMask(ResourceUsage usage) {
	switch(usage) {
		case ResourceUsage::Undefined:
		case ResourceUsage::PreInitialized:
		case ResourceUsage::Present:
		case ResourceUsage::General: return {};
		case ResourceUsage::RenderTarget: return vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
		case ResourceUsage::DepthStencil: return vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
		case ResourceUsage::ShaderResource: return vk::AccessFlagBits::eInputAttachmentRead;
		case ResourceUsage::CopySource: return vk::AccessFlagBits::eTransferRead;
		case ResourceUsage::CopyDestination: return vk::AccessFlagBits::eTransferWrite;
		case ResourceUsage::ShaderWrite: return vk::AccessFlagBits::eShaderWrite;
		default: return {};
	};
}

//-----------------

vk::ImageLayout getImageLayout(ResourceUsage usage) {
	switch(usage) {
		case ResourceUsage::Undefined: return vk::ImageLayout::eUndefined;
		case ResourceUsage::PreInitialized: return vk::ImageLayout::ePreinitialized;
		case ResourceUsage::ShaderWrite:
		case ResourceUsage::General: return vk::ImageLayout::eGeneral;
		case ResourceUsage::RenderTarget: return vk::ImageLayout::eColorAttachmentOptimal;
		case ResourceUsage::DepthStencil: return vk::ImageLayout::eDepthAttachmentStencilReadOnlyOptimal;
		case ResourceUsage::ShaderResource: return vk::ImageLayout::eShaderReadOnlyOptimal;
		case ResourceUsage::CopySource: return vk::ImageLayout::eTransferSrcOptimal;
		case ResourceUsage::CopyDestination: return vk::ImageLayout::eTransferDstOptimal;
		case ResourceUsage::Present: return vk::ImageLayout::ePresentSrcKHR;
		default: return vk::ImageLayout::eUndefined;
	};
}

//-----------------

vk::PipelineStageFlags getPipelineStageMask(ResourceUsage usage, bool src) {
	switch(usage) {
		case ResourceUsage::Undefined:
		case ResourceUsage::PreInitialized:
		case ResourceUsage::General: return src ? vk::PipelineStageFlagBits::eTopOfPipe : (vk::PipelineStageFlagBits::eAllGraphics | vk::PipelineStageFlagBits::eAllCommands);
		case ResourceUsage::RenderTarget: return vk::PipelineStageFlagBits::eColorAttachmentOutput;
		case ResourceUsage::DepthStencil: return src ? vk::PipelineStageFlagBits::eLateFragmentTests : vk::PipelineStageFlagBits::eEarlyFragmentTests;
		case ResourceUsage::ShaderWrite:
		case ResourceUsage::ShaderResource: return vk::PipelineStageFlagBits::eFragmentShader | vk::PipelineStageFlagBits::eComputeShader;
		case ResourceUsage::CopySource:
		case ResourceUsage::CopyDestination: return vk::PipelineStageFlagBits::eTransfer;
		case ResourceUsage::Present: return src ? (vk::PipelineStageFlagBits::eAllGraphics | vk::PipelineStageFlagBits::eAllCommands) : vk::PipelineStageFlagBits::eTopOfPipe;
		default: return vk::PipelineStageFlagBits::eAllGraphics | vk::PipelineStageFlagBits::eAllCommands;
	};
}

//-----------------

CommandBuffer::CommandBuffer(CommandPool* pool, bool primary) : pool(pool), primary(primary) { }

//-----------------

CommandBuffer::~CommandBuffer() {
	if(!handle) return;
	vk::Device vkDevice(handle);
	vk::CommandBuffer vkBuffer(handle);
	vk::CommandPool vkPool(pool->getApiHandle());
	vkDevice.freeCommandBuffers(vkPool, 1, &vkBuffer);
}

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

	handle = std::move(CommandBufferHandle(buffers.front(), vkDevice));
	if(handle)
		state = Initial;
	return handle;
}

//-----------------

void CommandBuffer::reset() {
	vk::CommandBuffer vkBuffer(handle);
	if(state == State::Recording)
		end();
	vkBuffer.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
	pipelineState.reset();
	state = State::Initial;
}

//-----------------

void CommandBuffer::free() {
	vk::CommandBuffer vkBuffer(handle);
	if(state == State::Recording)
		end();
	state = State::Free;
}


//-----------------

void CommandBuffer::flush(PipelineType bindPoint) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording. Call begin() first.",);
	vk::CommandBuffer vkBuffer(handle);
	auto device = pool->getDevice();

	// bind pipeline
	if(pipelineHash != pipelineState.getHash()) {
		auto pipeline = device->getPipelineCache()->requestPipeline(bindPoint, pipelineState);
		WARN_AND_RETURN_IF(!pipeline, "CommandBuffer: Could not create pipeline.",);
		pipelineHash = pipelineState.getHash();
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
		vkBuffer.bindPipeline(vkBindPoint, vkPipeline);
	}

	// bind descriptor sets
}

//-----------------

void CommandBuffer::begin() {
	WARN_AND_RETURN_IF(state == State::Recording, "Command buffer is already recording.",);
	WARN_AND_RETURN_IF(state == State::Free || state == State::Invalid, "Invalid command buffer.",);
	vk::CommandBuffer vkBuffer(handle);
	state = State::Recording;
	pipelineState.reset();
	vkBuffer.begin({vk::CommandBufferUsageFlagBits::eSimultaneousUse});
}

//-----------------

void CommandBuffer::end() {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording. Call begin() first.",);
	vk::CommandBuffer vkBuffer(handle);
	state = State::Executable;
	vkBuffer.end();
}

//-----------------

void CommandBuffer::beginRenderPass(const std::vector<Util::Color4f>& clearColors) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording. Call begin() first.",);
	vk::CommandBuffer vkBuffer(handle);
	auto& fbo = pipelineState.getFBO();
	WARN_AND_RETURN_IF(!fbo || !fbo->validate(), "Cannot begin render pass. Invalid FBO.",);
	vk::Framebuffer framebuffer(fbo->getApiHandle());
	vk::RenderPass renderPass(fbo->getRenderPass());

	std::vector<vk::ClearValue> clearValues(fbo->getColorAttachmentCount(), vk::ClearColorValue{});
	for(uint32_t i=0; i<clearValues.size(); ++i) {
		auto& c = clearColors[i];
		clearValues[i].color.setFloat32(std::array<float,4>{c.r(), c.g(), c.b(), c.a()});
	}

	vkBuffer.beginRenderPass({
		renderPass, framebuffer,
		vk::Rect2D{ {0, 0}, {fbo->getWidth(), fbo->getHeight()} },
		static_cast<uint32_t>(clearValues.size()), clearValues.data()
	}, vk::SubpassContents::eInline);
}

//-----------------

void CommandBuffer::endRenderPass() {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording. Call begin() first.",);
	vk::CommandBuffer vkBuffer(handle);
	vkBuffer.endRenderPass();
}

//-----------------

void CommandBuffer::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording. Call begin() first.",);
	if(instanceCount==0) return;
	flush(PipelineType::Graphics);
	vk::CommandBuffer vkBuffer(handle);
	vkBuffer.draw(vertexCount, instanceCount, firstVertex, firstInstance);
}

//-----------------

void CommandBuffer::textureBarrier(const TextureRef& texture, ResourceUsage newUsage) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording. Call begin() first.",);
	WARN_AND_RETURN_IF(!texture || !texture->isValid(), "Cannot create texture barrier. Invalid texture.",);
	if(texture->getLastUsage() == newUsage)
		return;

	vk::CommandBuffer vkBuffer(handle);
	auto image = texture->getImage();
	auto view = texture->getImageView();

	vk::ImageMemoryBarrier barrier{};
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.srcAccessMask = getAccessMask(texture->getLastUsage());
	barrier.dstAccessMask = getAccessMask(newUsage);
	barrier.oldLayout = getImageLayout(texture->getLastUsage());
	barrier.newLayout = getImageLayout(newUsage);
	barrier.image = image->getApiHandle();
	barrier.subresourceRange = { 
		vk::ImageAspectFlagBits::eColor, // TODO: check for depth/stencil format
		view->getMipLevel(), view->getMipLevelCount(),
		view->getLayer(), view->getLayerCount()
	};

	vkBuffer.pipelineBarrier(
		getPipelineStageMask(texture->getLastUsage(), true),
		getPipelineStageMask(newUsage, false),
		{}, {}, {}, {barrier}
	);
	texture->_setLastUsage(newUsage);
}

//-----------------

/*void CommandBuffer::bufferBarrier(const BufferObjectRef& buffer, ResourceUsage newUsage) {
	WARN_AND_RETURN_IF(!isRecording(), "Command buffer is not recording. Call begin() first.",);
	vk::CommandBuffer vkBuffer(handle);
}*/

//-----------------

} // namespace Rendering

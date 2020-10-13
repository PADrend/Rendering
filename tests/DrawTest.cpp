/*
	This file is part of the Rendering library.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "TestUtils.h"
#include <catch2/catch.hpp>

#include <Geometry/Box.h>
#include <Geometry/Vec3.h>
#include "../RenderingContext/RenderingContext.h"
#include "../RenderingContext/PipelineState.h"
#include "../Core/ApiHandles.h"
#include "../Core/Device.h"
#include "../Core/Queue.h"
#include "../Core/Swapchain.h"
#include "../Core/ImageStorage.h"
#include "../Core/ImageView.h"
#include "../Core/Pipeline.h"
#include "../FBO.h"
#include "../Texture/Texture.h"
#include "../Shader/Shader.h"
#include <Util/Timer.h>
#include <Util/Utils.h>
#include <cstdint>
#include <iostream>

#include <shaderc/shaderc.hpp>
#include <spirv_cross.hpp>
#include <vulkan/vulkan.hpp>

const std::string vertexShader = R"vs(
	#version 450

	out gl_PerVertex {
			vec4 gl_Position;
	};

	layout(location = 0) out vec3 fragColor;

	vec2 positions[3] = vec2[](
		vec2(0.0, -0.5),
		vec2(-0.5, 0.5),
		vec2(0.5, 0.5)
	);

	vec3 colors[3] = vec3[](
		vec3(1.0, 0.0, 0.0),
		vec3(0.0, 1.0, 0.0),
		vec3(0.0, 0.0, 1.0)
	);

	void main() {
		gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
		fragColor = colors[gl_VertexIndex];
	}
)vs";

const std::string fragmentShader = R"fs(
	#version 450

	layout(location = 0) in vec3 fragColor;

	layout(location = 0) out vec4 outColor;

	void main() {
		outColor = vec4(fragColor, 1.0);
	}
)fs";

TEST_CASE("DrawTest_testBox", "[DrawTest]") {
	using namespace Rendering;
	std::cout << std::endl;
	
	auto device = Device::create(TestUtils::window.get(), {"Test", 0u, 0u, true});
	vk::Device vkDevice(device->getApiHandle());
	
	// --------------------------------------------
	// create graphics pipeline
	
	// compile shaders
	auto shader = Shader::createShader(device, vertexShader, fragmentShader);
	REQUIRE(shader->init());

	auto pipeline = GraphicsPipeline::create(device, {}, shader);
	auto state = pipeline->getState();
	Geometry::Rect_i windowRect{0, 0, static_cast<int32_t>(TestUtils::window->getWidth()), static_cast<int32_t>(TestUtils::window->getHeight())};
	state.setViewportState({windowRect, windowRect});
	pipeline->setState(state);
		
	auto graphicsQueue = device->getQueue(Queue::Family::Graphics);
	auto swapchain = device->getSwapchain();
	
	// --------------------------------------------
	// command queue
	
	// create command buffers
	auto swapchainSize = swapchain->getSize();
	auto commandPool = vkDevice.createCommandPoolUnique({ {}, graphicsQueue->getFamilyIndex() });

	std::vector<vk::UniqueCommandBuffer> commandBuffers = vkDevice.allocateCommandBuffersUnique({
		*commandPool, 
		vk::CommandBufferLevel::ePrimary, 
		swapchainSize
	});
	
	for(uint32_t i=0; i<swapchainSize; ++i) {
		auto& fbo = swapchain->getFBO(i);
		vk::RenderPass renderPass(fbo->getRenderPass());
		vk::Framebuffer framebuffer(fbo->getApiHandle());
		auto& attachment = fbo->getColorTexture();
		auto& view = attachment->getImageView();
		auto& image = attachment->getImage();
		pipeline->setFBO(fbo);
		REQUIRE(pipeline->validate());
		vk::Pipeline vkPipeline(pipeline->getApiHandle());
		
		// record commands
		auto& cmdBuffer = commandBuffers[i];
		cmdBuffer->begin({vk::CommandBufferUsageFlagBits::eSimultaneousUse});
		
		vk::ImageMemoryBarrier barrier;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.oldLayout = vk::ImageLayout::eUndefined;
		barrier.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
		barrier.image = image->getApiHandle();
		barrier.subresourceRange = { vk::ImageAspectFlagBits::eColor,
			view->getMipLevel(), view->getMipLevelCount(),
			view->getLayer(), view->getLayerCount()
		};
		
		cmdBuffer->pipelineBarrier(
			vk::PipelineStageFlagBits::eAllGraphics | vk::PipelineStageFlagBits::eAllCommands, 
			vk::PipelineStageFlagBits::eAllGraphics | vk::PipelineStageFlagBits::eAllCommands, 
			{}, {}, {}, {barrier}
		);
		
		vk::ClearValue clearColor(vk::ClearColorValue{std::array<float,4>{0.0f, 0.0f, 0.0f, 1.0f}});
		cmdBuffer->beginRenderPass({
			renderPass, framebuffer,
			vk::Rect2D{ {0, 0}, {fbo->getWidth(), fbo->getHeight()} },
			1, &clearColor
		}, vk::SubpassContents::eInline);
		cmdBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, vkPipeline);
		cmdBuffer->draw(3, 1, 0, 0);
		
		cmdBuffer->endRenderPass();
				
		barrier.oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
		barrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
		
		cmdBuffer->pipelineBarrier(
			vk::PipelineStageFlagBits::eAllGraphics | vk::PipelineStageFlagBits::eAllCommands, 
			vk::PipelineStageFlagBits::eAllGraphics | vk::PipelineStageFlagBits::eAllCommands, 
			{}, {}, {}, {barrier}
		);
		
		cmdBuffer->end();
	}
	// --------------------------------------------
	// draw
			
	auto imageAvailableSemaphore = vkDevice.createSemaphoreUnique({});
	auto renderFinishedSemaphore = vkDevice.createSemaphoreUnique({});
	
	for(uint_fast32_t round = 0; round < 100; ++round) {
		auto index = swapchain->getCurrentIndex();

		vk::PipelineStageFlags waitStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;

		/*graphicsQueue.submit({{
			1, &imageAvailableSemaphore.get(), &waitStageMask, 
			1, &commandBuffers[index].get(), 
			1, &renderFinishedSemaphore.get() 
		}}, {});
		
		presentQueue.presentKHR({
			1, &renderFinishedSemaphore.get(),
			1, &swapchain,
			&index
		});*/
		vk::Queue queue(graphicsQueue->getApiHandle());
		queue.submit({{
			0, nullptr, &waitStageMask, 
			1, &commandBuffers[index].get(), 
			0, nullptr 
		}}, {});
		
		graphicsQueue->present();
	}
	vkDevice.waitIdle();
}
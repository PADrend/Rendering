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
#include "../Core/ApiHandles.h"
#include "../Core/Device.h"
#include "../Core/Queue.h"
#include "../Core/Swapchain.h"
#include "../Core/ImageStorage.h"
#include "../Core/ImageView.h"
#include "../FBO.h"
#include "../Texture/Texture.h"
#include "../Shader/ShaderObjectInfo.h"
#include <Util/Timer.h>
#include <Util/Utils.h>
#include <cstdint>
#include <iostream>

#include <shaderc/shaderc.hpp>
#include <spirv_cross.hpp>
#include <vulkan/vulkan.hpp>

const std::string vertexShader = R"vs(
	#version 450
	#extension GL_ARB_separate_shader_objects : enable

	out gl_PerVertex {
			vec4 gl_Position;
	};

	layout(location = 0) out vec3 fragColor;

	vec2 positions[3] = vec2[](
		vec2(0.0, -0.5),
		vec2(0.5, 0.5),
		vec2(-0.5, 0.5)
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
	#extension GL_ARB_separate_shader_objects : enable

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
	auto vso = ShaderObjectInfo::createVertex(vertexShader);
	auto fso = ShaderObjectInfo::createFragment(fragmentShader);
	auto vertexShaderModule = vso.compile(device);
	auto fragmentShaderModule = fso.compile(device);
	std::vector<vk::PipelineShaderStageCreateInfo> pipelineShaderStages = {
		{ {}, vk::ShaderStageFlagBits::eVertex, vk::ShaderModule(vertexShaderModule), "main" },
		{ {}, vk::ShaderStageFlagBits::eFragment, vk::ShaderModule(fragmentShaderModule), "main" }
	};
	
	vso.reflect();
	fso.reflect();
	
	// vertex input
	vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
	vk::PipelineInputAssemblyStateCreateInfo inputAssembly { {}, vk::PrimitiveTopology::eTriangleList, false };

	// rasterizer
	vk::Viewport viewport { 0.0f, 0.0f, static_cast<float>(TestUtils::window->getWidth()), static_cast<float>(TestUtils::window->getHeight()), 0.0f, 1.0f };
	vk::Rect2D scissor {{0, 0}, {TestUtils::window->getWidth(), TestUtils::window->getHeight()}};
	vk::PipelineViewportStateCreateInfo viewportState { {}, 1, &viewport, 1, &scissor };

	vk::PipelineRasterizationStateCreateInfo rasterizer;
	rasterizer.frontFace = vk::FrontFace::eClockwise;
	rasterizer.lineWidth = 1.0f;
	
	vk::PipelineMultisampleStateCreateInfo multisampling;

	// color blending
	std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments(device->getMaxFramebufferAttachments(), { 
		false,  
		vk::BlendFactor::eZero, vk::BlendFactor::eOne, vk::BlendOp::eAdd,
		vk::BlendFactor::eZero, vk::BlendFactor::eOne, vk::BlendOp::eAdd,
		vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
	});
	
	vk::PipelineColorBlendStateCreateInfo colorBlending { {}, false, vk::LogicOp::eCopy, static_cast<uint32_t>(colorBlendAttachments.size()), colorBlendAttachments.data() };
	
	// create pipeline	
	auto pipelineLayout = vkDevice.createPipelineLayoutUnique({});
	std::vector<vk::UniquePipeline> pipelines;
	/*auto pipeline = vkDevice.createGraphicsPipelineUnique({}, { {}, 
		static_cast<uint32_t>(pipelineShaderStages.size()), pipelineShaderStages.data(),
		&vertexInputInfo, &inputAssembly, nullptr, &viewportState, &rasterizer, &multisampling,
		nullptr, &colorBlending, nullptr, *pipelineLayout, renderPass, 0
	});*/
	
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
		
		pipelines.emplace_back(vkDevice.createGraphicsPipelineUnique({}, { {}, 
			static_cast<uint32_t>(pipelineShaderStages.size()), pipelineShaderStages.data(),
			&vertexInputInfo, &inputAssembly, nullptr, &viewportState, &rasterizer, &multisampling,
			nullptr, &colorBlending, nullptr, *pipelineLayout, renderPass, 0
		}));
		
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
		cmdBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, *pipelines.back());
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
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
#include "../Core/CommandBuffer.h"
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
	
	auto graphicsQueue = device->getQueue(QueueFamily::Graphics);
	auto swapchain = device->getSwapchain();

	// --------------------------------------------
	// create graphics pipeline
	
	// compile shaders
	auto shader = Shader::createShader(device, vertexShader, fragmentShader);
	REQUIRE(shader->init());

	PipelineState state{};
	Geometry::Rect_i windowRect{0, 0, static_cast<int32_t>(TestUtils::window->getWidth()), static_cast<int32_t>(TestUtils::window->getHeight())};
	state.setViewportState({windowRect, windowRect});
		
	// --------------------------------------------
	// command queue
	
	// create command buffers
	auto swapchainSize = swapchain->getSize();


	std::vector<CommandBuffer::Ref> commandBuffers;
	for(uint32_t i=0; i<swapchainSize; ++i) {
		auto& fbo = swapchain->getFBO(i);
		auto& attachment = fbo->getColorTexture();

		auto cmdBuffer = CommandBuffer::create(graphicsQueue);
		commandBuffers.emplace_back(cmdBuffer);
		
		// record commands
		cmdBuffer->begin();

		state.setFBO(fbo);
		cmdBuffer->setShader(shader);
		cmdBuffer->setPipelineState(state);
		cmdBuffer->textureBarrier(attachment, ResourceUsage::RenderTarget);

		cmdBuffer->beginRenderPass({{0,0,0,1}});
		cmdBuffer->draw(3);
		cmdBuffer->endRenderPass();
				
		cmdBuffer->textureBarrier(attachment, ResourceUsage::Present);
		cmdBuffer->end();
	}
	// --------------------------------------------
	// draw
	
	for(uint_fast32_t round = 0; round < 100; ++round) {
		auto index = swapchain->getCurrentIndex();
		graphicsQueue->submit(commandBuffers[index]);		
		graphicsQueue->present();
	}
	vkDevice.waitIdle();
}
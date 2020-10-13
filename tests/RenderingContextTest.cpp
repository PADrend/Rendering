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
#include <Geometry/Angle.h>
#include "../RenderingContext.h"
#include "../State/PipelineState.h"
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
#include "../BufferObject.h"
#include <Util/Timer.h>
#include <Util/Utils.h>
#include <cstdint>
#include <iostream>

#include <shaderc/shaderc.hpp>
#include <spirv_cross.hpp>
#include <vulkan/vulkan.hpp>

const std::string vertexShader = R"vs(
	#version 450

	layout(location = 0) in vec2 position;
	layout(location = 1) in vec4 color;

	layout(location = 0) out vec3 fragColor;

	layout(push_constant) uniform PushConstants {
		float angle;
	};

	void main() {
		float s = sin(angle);
		float c = cos(angle);
		mat2 m = mat2(c, -s, s, c);
		gl_Position = vec4(m * position, 0.0, 1.0);
		fragColor = color.rgb;
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

TEST_CASE("RenderingContext", "[RenderingContext]") {
	using namespace Rendering;
	std::cout << std::endl;
	
	auto device = TestUtils::device;
	REQUIRE(device);
	vk::Device vkDevice(device->getApiHandle());
	REQUIRE(vkDevice);
	
	auto graphicsQueue = device->getQueue(QueueFamily::Graphics);
	REQUIRE(graphicsQueue->supports(QueueFamily::Present));
	auto swapchain = device->getSwapchain();

	// --------------------------------------------
	// input

	std::vector<Geometry::Vec2> positions {
		{0.0, -0.5},
		{-0.5, 0.5},
		{0.5, 0.5}
	};

	std::vector<Util::Color4f> colors {
		{1.0, 0.0, 0.0},
		{0.0, 1.0, 0.0},
		{0.0, 0.0, 1.0}
	};

	auto vertexBuffer = BufferObject::create(device);
	vertexBuffer->allocate(positions.size() * sizeof(Geometry::Vec2) + colors.size() * sizeof(Util::Color4f), ResourceUsage::VertexBuffer);
	vertexBuffer->upload(positions);
	vertexBuffer->upload(colors, positions.size() * sizeof(Geometry::Vec2));

	// --------------------------------------------
	// create graphics pipeline
	
	// compile shaders
	auto shader = Shader::createShader(device, vertexShader, fragmentShader);
	REQUIRE(shader->init());

	PipelineState state{};
	Geometry::Rect_i windowRect{0, 0, static_cast<int32_t>(TestUtils::window->getWidth()), static_cast<int32_t>(TestUtils::window->getHeight())};
	state.setViewportState({windowRect, windowRect});

	VertexInputState inputState;
	inputState.setBindingCount(2);
	inputState.setBinding({0, sizeof(Geometry::Vec2)}, 0);
	inputState.setBinding({1, sizeof(Util::Color4f)}, 1);
	inputState.setAttributeCount(2);
	inputState.setAttribute({0, 0, InternalFormat::RG32Float, 0}, 0);
	inputState.setAttribute({1, 1, InternalFormat::RGBA32Float, 0}, 1);
	state.setVertexInputState(inputState);
	// --------------------------------------------
	// draw

	auto angle = Geometry::Angle::deg(0);
	for(uint_fast32_t round = 0; round < 1000; ++round) {
		auto& fbo = swapchain->getFBO(swapchain->getCurrentIndex());
		auto& attachment = fbo->getColorTexture();
		
		auto cmdBuffer = CommandBuffer::create(graphicsQueue);
		cmdBuffer->begin();
		cmdBuffer->setPipelineState(state);
		cmdBuffer->setShader(shader);
		cmdBuffer->setFBO(fbo);
		//cmdBuffer->textureBarrier(attachment, ResourceUsage::RenderTarget);

		cmdBuffer->beginRenderPass({{1,1,1,1}});
		cmdBuffer->bindVertexBuffers(0, {vertexBuffer, vertexBuffer}, {0, positions.size() * sizeof(Geometry::Vec2)});
		cmdBuffer->pushConstants(angle.deg());
		cmdBuffer->draw(3);
		cmdBuffer->endRenderPass();
				
		//cmdBuffer->textureBarrier(attachment, ResourceUsage::Present);
		cmdBuffer->end();

		graphicsQueue->submit(cmdBuffer);
		graphicsQueue->present();

		angle += Geometry::Angle::deg(1);
	}
	vkDevice.waitIdle();
}
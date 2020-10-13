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

const std::string shaderSrc = R"vs(
	#version 450
	#ifdef SG_VERTEX_SHADER

	vec2 positions[3] = vec2[](
		vec2(0.0, -0.5),
		vec2(-0.5, 0.5),
		vec2(0.5, 0.5)
	);

	void main() {
		gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
	}
	#endif
	#ifdef SG_FRAGMENT_SHADER

	layout(location = 0) out vec4 outColor;
	void main() {
		outColor = vec4(1.0, 0.0, 0.0, 1.0);
	}
	#endif
)vs";


TEST_CASE("FramebufferTest_testDraw", "[FramebufferTest]") {
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
	// create graphics pipeline
	
	// compile shaders
	auto shader = Shader::createShader(device, shaderSrc, shaderSrc);
	REQUIRE(shader->init());

	PipelineState state{};
	Geometry::Rect_i windowRect{0, 0, static_cast<int32_t>(TestUtils::window->getWidth()), static_cast<int32_t>(TestUtils::window->getHeight())};
	state.setViewportState({windowRect, windowRect});
	state.setShader(shader);
	state.setFramebufferFormat(swapchain->getCurrentFBO());

	// --------------------------------------------
	// draw

	for(uint_fast32_t round = 0; round < 100; ++round) {		
		auto cmdBuffer = CommandBuffer::create(graphicsQueue);

		cmdBuffer->beginRenderPass(nullptr, true, true, {{1,1,1,1}});
		cmdBuffer->draw(3);
		cmdBuffer->endRenderPass();
		
		cmdBuffer->prepareForPresent();

		graphicsQueue->submit(cmdBuffer);
		graphicsQueue->present();
	}
	vkDevice.waitIdle();
}
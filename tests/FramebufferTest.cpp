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
#include "../RenderDevice.h"
#include "../Shader/ShaderCompiler.h"
#include <Util/Timer.h>
#include <Util/Utils.h>

#include <nvrhi/utils.h>

#include <cstdint>
#include <iostream>

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
	auto frameContext = TestUtils::frameContext;
	REQUIRE(frameContext);
	auto nvDevice = device->_getInternalDevice();

	// --------------------------------------------
	// create graphics pipeline
	
	ShaderCompilerGLSL compiler;
	
	// compile shaders
	std::vector<uint32_t> vsSpirv, fsSpirv;
	REQUIRE(compiler.compile(shaderSrc, nvrhi::ShaderType::Vertex, vsSpirv));
	auto vertexShader = nvDevice->createShader({nvrhi::ShaderType::Vertex}, vsSpirv.data(), vsSpirv.size() * sizeof(uint32_t));
	REQUIRE(vertexShader);

	REQUIRE(compiler.compile(shaderSrc, nvrhi::ShaderType::Pixel, fsSpirv));
	auto pixelShader = nvDevice->createShader({nvrhi::ShaderType::Pixel}, fsSpirv.data(), fsSpirv.size() * sizeof(uint32_t));
	REQUIRE(pixelShader);

	// create framebuffer
	auto framebuffer = frameContext->getCurrentFramebuffer();
	REQUIRE(framebuffer);

	// create pipeline
	auto pipelineDesc = nvrhi::GraphicsPipelineDesc()
		.setVertexShader(vertexShader)
		.setPixelShader(pixelShader);

	auto graphicsPipeline = nvDevice->createGraphicsPipeline(pipelineDesc, framebuffer);
	REQUIRE(graphicsPipeline);
	
	nvrhi::Viewport viewport((float)TestUtils::window->getWidth(),	(float)TestUtils::window->getHeight());

	// --------------------------------------------
	// draw

	nvrhi::CommandListHandle commandList = nvDevice->createCommandList();
	for(uint_fast32_t round = 0; round < 100; ++round) {
		frameContext->beginFrame();
		auto currentFramebuffer = frameContext->getCurrentFramebuffer();

		commandList->open();

		// Clear the primary render target
		nvrhi::utils::ClearColorAttachment(commandList, currentFramebuffer, 0, nvrhi::Color(0.f));

		// Set the graphics state: pipeline, framebuffer, viewport, bindings.
		auto graphicsState = nvrhi::GraphicsState()
				.setPipeline(graphicsPipeline)
				.setFramebuffer(currentFramebuffer)
				.setViewport(nvrhi::ViewportState().addViewportAndScissorRect(viewport));
		commandList->setGraphicsState(graphicsState);

		// Draw our geometry
		auto drawArguments = nvrhi::DrawArguments()
				.setVertexCount(3);
		commandList->draw(drawArguments);

		// Close and execute the command list
		commandList->close();
		nvDevice->executeCommandList(commandList);
		frameContext->endFrame();
	}
	device->waitIdle();
}
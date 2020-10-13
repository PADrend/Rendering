/*
	This file is part of the Rendering library.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "TestUtils.h"
#include <catch2/catch.hpp>

#include "../Core/Device.h"
#include "../Core/Swapchain.h"
#include "../Core/ImageView.h"
#include "../Core/CommandBuffer.h"
#include "../State/PipelineState.h"
#include "../State/RenderingState.h"
#include "../State/BindingState.h"
#include "../FBO.h"
#include "../Shader/Shader.h"
#include "../Shader/ShaderUtils.h"
#include "../Texture/Texture.h"
#include "../Texture/TextureUtils.h"
#include "../Buffer/BufferObject.h"

using namespace Rendering;

TEST_CASE("StateTest_testPipelineState", "[StateTest]") {
	auto device = TestUtils::device;
	REQUIRE(device);
	auto shader = ShaderUtils::createDefaultShader(device);

	// --------------------------------------------

	PipelineState state1{};
	REQUIRE(state1.isDirty());
	state1.clearDirty();
	REQUIRE(!state1.isDirty());
	
	PipelineState state2{};
	state2.clearDirty();
	REQUIRE(!state2.isDirty());
	state2 = state1;
	REQUIRE(!state2.isDirty());
	state2.clearDirty();

	state1.setFramebufferFormat(device->getSwapchain()->getCurrentFBO());
	REQUIRE(state1.isDirty());
	state1.clearDirty();
	REQUIRE(!state1.isDirty());
	state2 = state1;
	REQUIRE(state2.isDirty());
	state2.clearDirty();
	REQUIRE(!state2.isDirty());
	state2 = state1;
	REQUIRE(!state2.isDirty());

	state1.getDepthStencilState().setDepthTestEnabled(false);
	REQUIRE(!state1.isDirty());
	state1.getDepthStencilState().setDepthTestEnabled(true);
	REQUIRE(state1.isDirty());

	state2.setShader(shader);
	REQUIRE(state2.isDirty());
	state2.clearDirty();
	REQUIRE(!state2.isDirty());
	state2.setShader(shader);
	REQUIRE(!state2.isDirty());

	auto cmdBuffer = CommandBuffer::create(device);
	REQUIRE(cmdBuffer);
	cmdBuffer->setShader(shader);
	REQUIRE(cmdBuffer->getPipeline().isDirty());
	cmdBuffer->flush();
	REQUIRE(!cmdBuffer->getPipeline().isDirty());
	cmdBuffer->setPipeline(state2);
	REQUIRE(cmdBuffer->getPipeline().isDirty());
	cmdBuffer->flush();
	REQUIRE(!cmdBuffer->getPipeline().isDirty());
	cmdBuffer->setPipeline(state2);
	REQUIRE(!cmdBuffer->getPipeline().isDirty());
	cmdBuffer->flush();
	REQUIRE(!cmdBuffer->getPipeline().isDirty());
}

TEST_CASE("StateTest_testRenderingState", "[StateTest]") {

	Geometry::Matrix4x4 mat;
	mat.rotate_deg(45, {1,0,0});
	mat.rotate_deg(45, {0,0,1});

	RenderingState state{};
	REQUIRE(state.getInstance().isDirty());
	state.getInstance().clearDirty();
	REQUIRE(!state.getInstance().isDirty());
	state.getInstance().setMatrixModelToCamera(mat);
	REQUIRE(state.getInstance().isDirty());
	state.getInstance().clearDirty();
	REQUIRE(!state.getInstance().isDirty());
	state.getInstance().setMatrixModelToCamera(mat);
	REQUIRE(!state.getInstance().isDirty());
	state.clearDirty();

	LightData light1{};
	light1.setPosition({1,2,3});
	light1.setDirection({0,-1,0});
	light1.setIntensity({4,5,6});
	light1.setType(LightType::Directional);
	REQUIRE(!state.getLights().isDirty());
	auto light1Id = state.getLights().addLight(light1);
	REQUIRE(state.getLights().isDirty());
	state.getLights().clearDirty();
	REQUIRE(light1Id == state.getLights().addLight(light1));
	REQUIRE(!state.getLights().isDirty());
	state.getLights().removeLight(light1);
	REQUIRE(state.getLights().isDirty());
	state.getLights().addLight(light1);
	REQUIRE(state.getLights().isDirty());
	state.clearDirty();
	REQUIRE(state.getLights().getLight(light1Id) == light1);

	RenderingState state2{};
	REQUIRE(state2.isDirty());
	state2.clearDirty();
	REQUIRE(!state2.isDirty());
	state2 = state;
	REQUIRE(state2.isDirty());
	state2.clearDirty();
	state2 = state;
	REQUIRE(!state2.isDirty());
}


TEST_CASE("StateTest_testBindingState", "[StateTest]") {

	auto bo1 = BufferObject::create(TestUtils::device);
	auto bo2 = BufferObject::create(TestUtils::device);
	auto tex1 = TextureUtils::createStdTexture(16,16,true);
	auto shader = ShaderUtils::createDefaultShader(TestUtils::device);
	auto cmdBuffer = CommandBuffer::create(TestUtils::device);
	cmdBuffer->setShader(shader);

	BindingState state{};
	REQUIRE(state.isDirty());
	state.clearDirty();
	REQUIRE(!state.isDirty());

	state.bind(bo1, 0, 1, 2);
	REQUIRE(state.isDirty());
	REQUIRE(state.hasBinding(0, 1, 2));
	REQUIRE(state.getBinding(0, 1, 2).getBuffer() == bo1);
	state.clearDirty();
	REQUIRE(!state.isDirty());

	state.bind(bo2, 1, 2, 3);
	REQUIRE(state.isDirty());
	REQUIRE(!state.getBindingSet(0).isDirty());
	REQUIRE(state.getBindingSet(1).isDirty());
	REQUIRE(state.getBinding(0, 1, 2).getBuffer() == bo1);
	REQUIRE(state.getBinding(1, 2, 3).getBuffer() == bo2);
	REQUIRE(!state.hasBinding(1, 2, 2));
	REQUIRE(!state.hasBinding(0, 0, 0));
	REQUIRE(!state.hasBindingSet(2));
	state.clearDirty();
	REQUIRE(!state.isDirty());
	REQUIRE(!state.getBindingSet(0).isDirty());
	REQUIRE(!state.getBindingSet(1).isDirty());
	REQUIRE(!state.getBinding(1, 2, 3).isDirty());
	REQUIRE(!state.getBinding(0, 1, 2).isDirty());

	state.bind(tex1, 0, 1, 2);
	REQUIRE(state.isDirty());
	REQUIRE(state.getBindingSet(0).isDirty());
	REQUIRE(!state.getBindingSet(1).isDirty());
	REQUIRE(state.getBinding(0, 1, 2).getBuffer() == nullptr);
	REQUIRE(state.getBinding(0, 1, 2).getTexture() == tex1);
	REQUIRE(state.getBinding(1, 2, 3).getBuffer() == bo2);
	state.clearDirty();
	REQUIRE(!state.isDirty());

	BindingState state2{};
	REQUIRE(state != state2);
	state2.bind(tex1, 0, 1, 2);
	REQUIRE(state != state2);
	state2.bind(bo2, 1, 2, 3);
	REQUIRE(state == state2);
	REQUIRE(state2.isDirty());
	REQUIRE(!state.isDirty());
	state = state2;
	REQUIRE(!state.isDirty());
	REQUIRE(!state.getBindingSet(0).isDirty());
	REQUIRE(!state.getBindingSet(1).isDirty());
	REQUIRE(state.getBinding(0, 1, 2).getBuffer() == nullptr);
	REQUIRE(state.getBinding(0, 1, 2).getTexture() == tex1);
	REQUIRE(state.getBinding(1, 2, 3).getBuffer() == bo2);
	REQUIRE(state.getBinding(1, 2, 3).getTexture() == nullptr);
	state.clearDirty();

	REQUIRE(state2.hasBindingSet(0));
	REQUIRE(state2.hasBindingSet(1));
	state.reset();
	state.bind(tex1, 0, 1, 2);
	REQUIRE(state.isDirty());
	state.clearDirty();
	state = state2;
	REQUIRE(state.isDirty());
	REQUIRE(!state.getBindingSet(0).isDirty());
	REQUIRE(state.hasBindingSet(1));
	REQUIRE(state.getBindingSet(1).isDirty());
	state.clearDirty();

	state2.reset();
	state2.bind(tex1, 0, 1, 2);
	REQUIRE(!state2.hasBindingSet(1));
	state = state2;
	REQUIRE(state.isDirty());
	REQUIRE(!state.getBindingSet(0).isDirty());
	REQUIRE(!state.hasBindingSet(1));
	state.bind(bo2, 1, 2, 3);
	REQUIRE(state.isDirty());
	state2 = state;
	state.clearDirty();
	state2.clearDirty();

	REQUIRE(cmdBuffer->getBindings().isDirty());
	cmdBuffer->flush();
	REQUIRE(!cmdBuffer->getBindings().isDirty());
	cmdBuffer->setBindings(state);
	REQUIRE(cmdBuffer->getBindings().isDirty());
	cmdBuffer->flush();
	REQUIRE(!cmdBuffer->getBindings().isDirty());
	cmdBuffer->setBindings(state);
	REQUIRE(!cmdBuffer->getBindings().isDirty());
	cmdBuffer->bindBuffer(bo2, 1, 2, 3);
	REQUIRE(!cmdBuffer->getBindings().isDirty());
	cmdBuffer->bindBuffer(bo1, 1, 3, 0);
	REQUIRE(cmdBuffer->getBindings().isDirty());
	cmdBuffer->flush();
	REQUIRE(!cmdBuffer->getBindings().isDirty());

	state.bind(tex1, 0, 0, 0);
	REQUIRE(state.isDirty());
	REQUIRE(state.getBindingSet(0).isDirty());
	REQUIRE(!state.getBindingSet(1).isDirty());
	state2 = state;
	REQUIRE(state2.isDirty());
	REQUIRE(state2.getBindingSet(0).isDirty());
	REQUIRE(!state2.getBindingSet(1).isDirty());

	state.clearDirty();
	REQUIRE(state.hasBindingSet(0));
	REQUIRE(state.hasBindingSet(1));
	REQUIRE(!state.hasBindingSet(2));
	REQUIRE(state.getBinding(0,0,0).getTexture() == tex1);
	REQUIRE(state.getBinding(0,1,2).getTexture() == tex1);
	REQUIRE(state.getBinding(1,2,3).getBuffer() == bo2);
	REQUIRE(!cmdBuffer->getBindings().hasBinding(0,0,0));
	REQUIRE(cmdBuffer->getBindings().getBinding(0,1,2).getTexture() == tex1);
	REQUIRE(cmdBuffer->getBindings().getBinding(1,2,3).getBuffer() == bo2);
	REQUIRE(cmdBuffer->getBindings().getBinding(1,3,0).getBuffer() == bo1);
	cmdBuffer->setBindings(state);
	REQUIRE(cmdBuffer->getBindings() == state);
	REQUIRE(cmdBuffer->getBindings().isDirty());
	REQUIRE(cmdBuffer->getBindings().getBindingSet(0).isDirty());
	REQUIRE(cmdBuffer->getBindings().getBindingSet(1).isDirty());
}
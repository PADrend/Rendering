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
#include "../State/PipelineState.h"
#include "../State/RenderingState.h"
#include "../FBO.h"
#include "../Shader/Shader.h"
#include "../Texture/Texture.h"

using namespace Rendering;

TEST_CASE("StateTest_testPipelineState", "[StateTest]") {
	auto device = TestUtils::device;
	REQUIRE(device);

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

	device->waitIdle();
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
}
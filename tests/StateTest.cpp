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
#include "../FBO.h"
#include "../Shader/Shader.h"

TEST_CASE("StateTest_testPipelineState", "[StateTest]") {
	using namespace Rendering;
	std::cout << std::endl;
	
	auto device = TestUtils::device;
	REQUIRE(device);

	// --------------------------------------------

	PipelineState state1{};
	REQUIRE(state1.hasChanged());
	state1.markAsUnchanged();
	REQUIRE(!state1.hasChanged());
	
	PipelineState state2{};
	state2.markAsUnchanged();
	REQUIRE(!state2.hasChanged());
	state2 = state1;
	REQUIRE(!state2.hasChanged());
	state2.markAsUnchanged();

	state1.setFramebufferFormat(device->getSwapchain()->getCurrentFBO());
	REQUIRE(state1.hasChanged());
	state1.markAsUnchanged();
	REQUIRE(!state1.hasChanged());
	state2 = state1;
	REQUIRE(state2.hasChanged());
	state2.markAsUnchanged();
	REQUIRE(!state2.hasChanged());
	state2 = state1;
	REQUIRE(!state2.hasChanged());

	device->waitIdle();
}
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
#include "../Core/Device.h"
#include <Util/Timer.h>
#include <Util/Utils.h>
#include <cstdint>
#include <iostream>

TEST_CASE("DeviceTest_test", "[DeviceTest]") {
	using namespace Rendering;
	std::cout << std::endl;
	
	auto device = TestUtils::device;
	REQUIRE(device);
	REQUIRE(device->getApiHandle());
	std::cout << "Max. push constant size: " << device->getMaxPushConstantSize() << std::endl;
	REQUIRE(device->getMaxPushConstantSize() >= 128);
	std::cout << "Max. framebuffer attachments: " << device->getMaxFramebufferAttachments() << std::endl;
	REQUIRE(device->getMaxFramebufferAttachments() >= 1);
	device->waitIdle();
}

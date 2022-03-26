/*
	This file is part of the Rendering library.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "TestUtils.h"
#include "../Vulkan/VulkanDevice.h"
#include "../Vulkan/VulkanFrameContext.h"
#include <Util/UI/UI.h>
#include <Util/UI/Window.h>
#include <Util/Util.h>
#include <Util/Macros.h>
#include <memory>
#include <vector>

#undef WARN
#undef INFO
#undef FAIL
#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>

Util::Reference<Util::UI::Window> TestUtils::window;
Rendering::RenderDeviceHandle TestUtils::device;
Rendering::RenderFrameContextHandle TestUtils::frameContext;
//std::unique_ptr<Rendering::RenderingContext> TestUtils::context;

int main( int argc, char* argv[] ) {
	using namespace Rendering;
  Util::init();

	Util::UI::Window::Properties properties;
	properties.positioned = false;
	properties.clientAreaWidth = 1280;
	properties.clientAreaHeight = 720;
	properties.title = "Rendering Test";
	properties.compatibilityProfile = true;
	properties.debug = true;
	properties.renderingAPI = Util::UI::Window::Properties::VULKAN;
	properties.contextVersionMajor = 1;
	properties.contextVersionMinor = 2;
	TestUtils::window = Util::UI::createWindow(properties);

	VulkanInstanceConfig instanceConfig{};
	instanceConfig.name = "Vulkan Test Instance";
	instanceConfig.apiVersionMajor = 1;
	instanceConfig.apiVersionMinor = 2;
	instanceConfig.debug = true;
	instanceConfig.validationLayers = {
	};

	
	FAIL_IF(!VulkanInstance::init(instanceConfig));
	auto physicalDevices = VulkanInstance::getPhysicalDevices();

	VulkanDeviceConfig config(physicalDevices.front());
	config.extensions = {
		"VK_KHR_swapchain" // required for window support
	};

	TestUtils::device = VulkanDevice::create(config);
	FAIL_IF(!TestUtils::device);
	TestUtils::frameContext = TestUtils::device->createFrameContext(TestUtils::window);
	FAIL_IF(!TestUtils::frameContext);
	
	auto result = Catch::Session().run( argc, argv );
	
	TestUtils::frameContext = nullptr;
	TestUtils::device = nullptr;
	VulkanInstance::shutdown();
	TestUtils::window = nullptr; // FIXME: Needs to be called after instance shutdown or glfw unloads the vulkan library.
	return result;
}

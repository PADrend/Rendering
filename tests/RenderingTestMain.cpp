/*
	This file is part of the Rendering library.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "TestUtils.h"
#include <Util/UI/UI.h>
#include <Util/UI/Window.h>
#include <Util/Util.h>

#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>

#include <memory>
#include <vector>

Util::Reference<Util::UI::Window> TestUtils::window;
Rendering::Device::Ref TestUtils::device;
std::unique_ptr<Rendering::RenderingContext> TestUtils::context;

int main( int argc, char* argv[] ) {
  Util::init();

	Util::UI::Window::Properties properties;
	properties.positioned = false;
	properties.clientAreaWidth = 512;
	properties.clientAreaHeight = 512;
	properties.title = "Rendering Test";
	properties.compatibilityProfile = true;
	properties.debug = true;
	TestUtils::window = Util::UI::createWindow(properties);
	TestUtils::device = Rendering::Device::create(TestUtils::window.get(), {
		//"VK_LAYER_LUNARG_api_dump"
		//"VK_LAYER_RENDERDOC_Capture"
		"VK_LAYER_LUNARG_monitor"
	}, true);
	//TestUtils::context.reset(new Rendering::RenderingContext(TestUtils::device));
	
	auto result = Catch::Session().run( argc, argv );
	
	TestUtils::device->waitIdle();
	std::cout << "Device References: " << TestUtils::device->countReferences() << std::endl;
	TestUtils::device = nullptr;
	TestUtils::window = nullptr;
	return result;
}

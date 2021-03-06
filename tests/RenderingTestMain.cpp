/*
	This file is part of the Rendering library.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "TestUtils.h"
#include "../RenderingContext/RenderingContext.h"
#include "../Helper.h"
#include <Util/UI/UI.h>
#include <Util/UI/Window.h>
#include <Util/Util.h>

#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>

#include <memory>

Util::Reference<Util::UI::Window> TestUtils::window;

int main( int argc, char* argv[] ) {
  Util::init();

	Util::UI::Window::Properties properties;
	properties.positioned = false;
	properties.clientAreaWidth = 256;
	properties.clientAreaHeight = 256;
	properties.title = "Rendering Test";
	properties.compatibilityProfile = true;
	TestUtils::window = Util::UI::createWindow(properties);
	Rendering::enableGLErrorChecking();
	Rendering::RenderingContext::initGLState();
	
	auto result = Catch::Session().run( argc, argv );
	
	TestUtils::window = nullptr;
	return result;
}

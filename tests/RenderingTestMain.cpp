/*
	This file is part of the Rendering library.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "TestUtils.h"
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Helper.h>
#include <Util/UI/UI.h>
#include <Util/UI/Window.h>
#include <Util/Util.h>
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestRunner.h>
#include <cppunit/TextOutputter.h>
#include <cppunit/XmlOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <fstream>
#include <memory>

std::unique_ptr<Util::UI::Window> TestUtils::window;

int main(int /*argc*/, char ** /*argv*/) {
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

	CppUnit::TestResult controller;

	CppUnit::TestResultCollector result;
	controller.addListener(&result);

	CppUnit::BriefTestProgressListener progress;
	controller.addListener(&progress);

	CppUnit::TestRunner runner;
	runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

	runner.run(controller);

	std::ofstream fileStream("cppunit_results.xml");
	CppUnit::XmlOutputter xmlOutput(&result, fileStream, "UTF-8");
	xmlOutput.write();

	CppUnit::TextOutputter textOutput(&result, std::cout);
	textOutput.write();

	TestUtils::window.release();

	return 0;
}

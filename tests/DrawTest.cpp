/*
	This file is part of the Rendering library.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "DrawTest.h"
#include <cppunit/TestAssert.h>
#include <Geometry/Box.h>
#include <Geometry/Vec3.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Draw.h>
#include <Rendering/Helper.h>
#include <Util/Timer.h>
#include <cstdint>
#include <iostream>
CPPUNIT_TEST_SUITE_REGISTRATION(DrawTest);

void DrawTest::testBox() {
	using namespace Rendering;
	std::cout << std::endl;

	const Geometry::Box boxA(Geometry::Vec3f(2.0f, 2.0f, 2.0f), 3.0f);
	const Geometry::Box boxB(Geometry::Vec3f(-5.0f, -5.0f, -5.0f), 1.0f);
	const Geometry::Box boxC(Geometry::Vec3f(17.0f, 17.0f, 17.0f), 12.0f);

	RenderingContext context;
	context.setImmediateMode(false);
	Rendering::disableGLErrorChecking();

	Util::Timer drawFastBoxTimer;
	Util::Timer drawBoxTimer;
	for(uint_fast32_t round = 0; round < 1000; ++round) {
		drawFastBoxTimer.resume();
		context.applyChanges();
		for(uint_fast32_t box = 0; box < 1000; ++box) {
			drawFastAbsBox(context, boxA);
			drawFastAbsBox(context, boxB);
			drawFastAbsBox(context, boxC);
		}
		drawFastBoxTimer.stop();
		drawBoxTimer.resume();
		for(uint_fast32_t box = 0; box < 1000; ++box) {
			drawAbsBox(context, boxA);
			drawAbsBox(context, boxB);
			drawAbsBox(context, boxC);
		}
		drawBoxTimer.stop();
	}
	std::cout << "drawFastAbsBox: " << drawFastBoxTimer.getSeconds() << " s" << std::endl;
	std::cout << "drawAbsBox: " << drawBoxTimer.getSeconds() << " s" << std::endl;
}

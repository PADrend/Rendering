/*
	This file is part of the Rendering library.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_DRAWTEST_H
#define RENDERING_DRAWTEST_H

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

class DrawTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(DrawTest);
	CPPUNIT_TEST(testBox);
	CPPUNIT_TEST_SUITE_END();

	public:
		void testBox();
};

#endif /* RENDERING_DRAWTEST_H */

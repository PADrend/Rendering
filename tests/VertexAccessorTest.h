/*
 This file is part of the Rendering library.
 Copyright (C) 2018 Sascha Brandt <sascha@brandt.graphics>
 
 This library is subject to the terms of the Mozilla Public License, v. 2.0.
 You should have received a copy of the MPL along with this library; see the
 file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_TESTS_VERTEXACCESSORTEST_H_
#define RENDERING_TESTS_VERTEXACCESSORTEST_H_

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

class VertexAccessorTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(VertexAccessorTest);
	CPPUNIT_TEST(compareSpeed);
	CPPUNIT_TEST_SUITE_END();

	public:
		void compareSpeed();
};

#endif /* end of include guard: RENDERING_TESTS_VERTEXACCESSORTEST_H_ */

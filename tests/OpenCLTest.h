/*
	This file is part of the Rendering library.
	Copyright (C) 2014 Sascha Brandt <myeti@mail.upb.de>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef RENDERING_HAS_LIB_OPENCL

#ifndef OPENCLTEST_H_
#define OPENCLTEST_H_

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

class OpenCLTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(OpenCLTest);
	CPPUNIT_TEST(test);
	CPPUNIT_TEST(interopTest);
	CPPUNIT_TEST(textureGLFilterTest);
	CPPUNIT_TEST(bitmapFilterTest);
	CPPUNIT_TEST(nativeKernelTest);
	CPPUNIT_TEST_SUITE_END();
public:
	void test();
	void nativeKernelTest();
	void interopTest();
	void textureGLFilterTest();
	void bitmapFilterTest();
};

#endif /* OPENCLTEST_H_ */
#endif /* RENDERING_HAS_LIB_OPENCL */

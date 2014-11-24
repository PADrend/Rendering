/*
 * OpenCLTest.h
 *
 *  Created on: Nov 11, 2014
 *      Author: sascha
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

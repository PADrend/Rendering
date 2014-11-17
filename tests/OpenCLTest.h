/*
 * OpenCLTest.h
 *
 *  Created on: Nov 11, 2014
 *      Author: sascha
 */

#ifdef HAVE_LIB_OPENCL

#ifndef OPENCLTEST_H_
#define OPENCLTEST_H_

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

class OpenCLTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(OpenCLTest);
	CPPUNIT_TEST(test);
	CPPUNIT_TEST(interopTest);
	CPPUNIT_TEST_SUITE_END();
public:
	void test();
	void interopTest();
};

#endif /* OPENCLTEST_H_ */
#endif /* HAVE_LIB_OPENCL */

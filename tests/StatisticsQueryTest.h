/*
	This file is part of the Rendering library.
	Copyright (C) 2016 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_STATISTICSQUERYTEST_H
#define RENDERING_STATISTICSQUERYTEST_H

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

class StatisticsQueryTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(StatisticsQueryTest);
	CPPUNIT_TEST(testVerticesSubmittedQuery);
	CPPUNIT_TEST(testPrimitivesSubmittedQuery);
	CPPUNIT_TEST(testVertexShaderInvocationsQuery);
	CPPUNIT_TEST(testTessControlShaderPatchesQuery);
	CPPUNIT_TEST(testTessEvaluationShaderInvocationsQuery);
	CPPUNIT_TEST(testGeometryShaderInvocationsQuery);
	CPPUNIT_TEST(testGeometryShaderPrimitivesEmittedQuery);
	CPPUNIT_TEST(testFragmentShaderInvocationsQuery);
	CPPUNIT_TEST(testComputeShaderInvocationsQuery);
	CPPUNIT_TEST(testClippingInputPrimitivesQuery);
	CPPUNIT_TEST(testClippingOutputPrimitivesQuery);
	CPPUNIT_TEST_SUITE_END();

	public:
		void testVerticesSubmittedQuery();
		void testPrimitivesSubmittedQuery();
		void testVertexShaderInvocationsQuery();
		void testTessControlShaderPatchesQuery();
		void testTessEvaluationShaderInvocationsQuery();
		void testGeometryShaderInvocationsQuery();
		void testGeometryShaderPrimitivesEmittedQuery();
		void testFragmentShaderInvocationsQuery();
		void testComputeShaderInvocationsQuery();
		void testClippingInputPrimitivesQuery();
		void testClippingOutputPrimitivesQuery();
};

#endif /* RENDERING_STATISTICSQUERYTEST_H */

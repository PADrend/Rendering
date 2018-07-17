/*
	This file is part of the Rendering library.
	Copyright (C) 2016 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "StatisticsQueryTest.h"
#include <cppunit/TestAssert.h>
#include <Geometry/Box.h>
#include <Geometry/Vec3.h>
#include <Rendering/RenderingContext/RenderingContext.h>
#include <Rendering/Draw.h>
#include <Rendering/StatisticsQuery.h>
#include <Rendering/Shader/Shader.h>
#include <Rendering/Shader/ShaderUtils.h>
CPPUNIT_TEST_SUITE_REGISTRATION(StatisticsQueryTest);

static const Geometry::Box box(Geometry::Vec3f(2.0f, 2.0f, 2.0f), 3.0f);

using namespace Rendering;

static void testEmptyStatisticsQuery(StatisticsQuery & query, const uint32_t expectedResult) {
	CPPUNIT_ASSERT(query.isValid());
	query.begin();
	query.end();
	CPPUNIT_ASSERT_EQUAL(expectedResult, query.getResult());
}

static void testBoxStatisticsQuery(StatisticsQuery & query, const uint32_t expectedResult) {
	CPPUNIT_ASSERT(query.isValid());
	query.begin();
	RenderingContext context;
	context.setShader(ShaderUtils::createPassThroughShader().detachAndDecrease());
	drawBox(context, box);
	query.end();
	CPPUNIT_ASSERT_EQUAL(expectedResult, query.getResult());
}

void StatisticsQueryTest::testVerticesSubmittedQuery() {
	auto query = StatisticsQuery::createVerticesSubmittedQuery();
	testEmptyStatisticsQuery(query, 0);
	// 6 sides, 2 triangles per side, 3 vertices per triangle
	testBoxStatisticsQuery(query, 36);
}

void StatisticsQueryTest::testPrimitivesSubmittedQuery() {
	auto query = StatisticsQuery::createPrimitivesSubmittedQuery();
	testEmptyStatisticsQuery(query, 0);
	// 6 sides, 2 triangles per side
	testBoxStatisticsQuery(query, 12);
}

void StatisticsQueryTest::testVertexShaderInvocationsQuery() {
	auto query = StatisticsQuery::createVertexShaderInvocationsQuery();
	testEmptyStatisticsQuery(query, 0);
	// 6 sides, 4 vertices per side
	testBoxStatisticsQuery(query, 24);
}

void StatisticsQueryTest::testTessControlShaderPatchesQuery() {
	auto query = StatisticsQuery::createTessControlShaderPatchesQuery();
	testEmptyStatisticsQuery(query, 0);
	// No tesselation shader
	testBoxStatisticsQuery(query, 0);
}

void StatisticsQueryTest::testTessEvaluationShaderInvocationsQuery() {
	auto query = StatisticsQuery::createTessEvaluationShaderInvocationsQuery();
	testEmptyStatisticsQuery(query, 0);
	// No tesselation shader
	testBoxStatisticsQuery(query, 0);
}

void StatisticsQueryTest::testGeometryShaderInvocationsQuery() {
	auto query = StatisticsQuery::createGeometryShaderInvocationsQuery();
	testEmptyStatisticsQuery(query, 0);
	// No geometry shader
	testBoxStatisticsQuery(query, 0);
}

void StatisticsQueryTest::testGeometryShaderPrimitivesEmittedQuery() {
	auto query = StatisticsQuery::createGeometryShaderPrimitivesEmittedQuery();
	testEmptyStatisticsQuery(query, 0);
	// No geometry shader
	testBoxStatisticsQuery(query, 0);
}

void StatisticsQueryTest::testFragmentShaderInvocationsQuery() {
	auto query = StatisticsQuery::createFragmentShaderInvocationsQuery();
	testEmptyStatisticsQuery(query, 0);
	// Box not visible
	testBoxStatisticsQuery(query, 0);
}

void StatisticsQueryTest::testComputeShaderInvocationsQuery() {
	auto query = StatisticsQuery::createComputeShaderInvocationsQuery();
	testEmptyStatisticsQuery(query, 0);
	// No compute shader
	testBoxStatisticsQuery(query, 0);
}

void StatisticsQueryTest::testClippingInputPrimitivesQuery() {
	auto query = StatisticsQuery::createClippingInputPrimitivesQuery();
	testEmptyStatisticsQuery(query, 0);
	// 6 sides, 2 triangles per side
	testBoxStatisticsQuery(query, 12);
}

void StatisticsQueryTest::testClippingOutputPrimitivesQuery() {
	auto query = StatisticsQuery::createClippingOutputPrimitivesQuery();
	testEmptyStatisticsQuery(query, 0);
	// 4 triangles clipped (result of running this test)
	testBoxStatisticsQuery(query, 8);
}

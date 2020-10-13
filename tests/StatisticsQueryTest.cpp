/*
	This file is part of the Rendering library.
	Copyright (C) 2016 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "TestUtils.h"

#include <catch2/catch.hpp>

#include <Geometry/Box.h>
#include <Geometry/Vec3.h>
#include <Rendering/Context/RenderingContext.h>
#include <Rendering/Texture/Texture.h>
#include <Rendering/Draw.h>
#include <Rendering/StatisticsQuery.h>

#define REQUIRE_EQUAL(a,b) REQUIRE((a) == (b))

static const Geometry::Box box(Geometry::Vec3f(2.0f, 2.0f, 2.0f), 3.0f);

using namespace Rendering;

static void testEmptyStatisticsQuery(StatisticsQuery & query, const uint32_t expectedResult) {
	SECTION("testEmptyStatisticsQuery") {
		RenderingContext& context = *TestUtils::context.get();
		context.setTexture(0, nullptr);
		REQUIRE(query.isValid());
		query.begin(context);
		query.end(context);
		REQUIRE_EQUAL(expectedResult, query.getResult(context));
	}
}

static void testBoxStatisticsQuery(StatisticsQuery & query, const uint32_t expectedResult) {
	SECTION("testBoxStatisticsQuery") {
		RenderingContext& context = *TestUtils::context.get();
		context.setTexture(0, nullptr);
		REQUIRE(query.isValid());
		query.begin(context);
		drawBox(context, box);
		query.end(context);
		REQUIRE_EQUAL(expectedResult, query.getResult(context));
	}
}

TEST_CASE("StatisticsQueryTest_testVerticesSubmittedQuery", "[StatisticsQueryTest]") {
	auto query = StatisticsQuery::createVerticesSubmittedQuery();
	testEmptyStatisticsQuery(query, 0);
	// 6 sides, 2 triangles per side, 3 vertices per triangle
	testBoxStatisticsQuery(query, 36);
}

TEST_CASE("StatisticsQueryTest_testPrimitivesSubmittedQuery", "[StatisticsQueryTest]") {
	auto query = StatisticsQuery::createPrimitivesSubmittedQuery();
	testEmptyStatisticsQuery(query, 0);
	// 6 sides, 2 triangles per side
	testBoxStatisticsQuery(query, 12);
}

TEST_CASE("StatisticsQueryTest_testVertexShaderInvocationsQuery", "[StatisticsQueryTest]") {
	auto query = StatisticsQuery::createVertexShaderInvocationsQuery();
	testEmptyStatisticsQuery(query, 0);
	// 6 sides, 4 vertices per side
	testBoxStatisticsQuery(query, 24);
}

TEST_CASE("StatisticsQueryTest_testTessControlShaderPatchesQuery", "[StatisticsQueryTest]") {
	auto query = StatisticsQuery::createTessControlShaderPatchesQuery();
	testEmptyStatisticsQuery(query, 0);
	// No tesselation shader
	testBoxStatisticsQuery(query, 0);
}

TEST_CASE("StatisticsQueryTest_testTessEvaluationShaderInvocationsQuery", "[StatisticsQueryTest]") {
	auto query = StatisticsQuery::createTessEvaluationShaderInvocationsQuery();
	testEmptyStatisticsQuery(query, 0);
	// No tesselation shader
	testBoxStatisticsQuery(query, 0);
}

TEST_CASE("StatisticsQueryTest_testGeometryShaderInvocationsQuery", "[StatisticsQueryTest]") {
	auto query = StatisticsQuery::createGeometryShaderInvocationsQuery();
	testEmptyStatisticsQuery(query, 0);
	// No geometry shader
	testBoxStatisticsQuery(query, 0);
}

TEST_CASE("StatisticsQueryTest_testGeometryShaderPrimitivesEmittedQuery", "[StatisticsQueryTest]") {
	auto query = StatisticsQuery::createGeometryShaderPrimitivesEmittedQuery();
	testEmptyStatisticsQuery(query, 0);
	// No geometry shader
	testBoxStatisticsQuery(query, 0);
}

TEST_CASE("StatisticsQueryTest_testFragmentShaderInvocationsQuery", "[StatisticsQueryTest]") {
	auto query = StatisticsQuery::createFragmentShaderInvocationsQuery();
	testEmptyStatisticsQuery(query, 0);
	// Box not visible
	testBoxStatisticsQuery(query, 0);
}

TEST_CASE("StatisticsQueryTest_testComputeShaderInvocationsQuery", "[StatisticsQueryTest]") {
	auto query = StatisticsQuery::createComputeShaderInvocationsQuery();
	testEmptyStatisticsQuery(query, 0);
	// No compute shader
	testBoxStatisticsQuery(query, 0);
}

TEST_CASE("StatisticsQueryTest_testClippingInputPrimitivesQuery", "[StatisticsQueryTest]") {
	auto query = StatisticsQuery::createClippingInputPrimitivesQuery();
	testEmptyStatisticsQuery(query, 0);
	// 6 sides, 2 triangles per side
	testBoxStatisticsQuery(query, 12);
}

TEST_CASE("StatisticsQueryTest_testClippingOutputPrimitivesQuery", "[StatisticsQueryTest]") {
	auto query = StatisticsQuery::createClippingOutputPrimitivesQuery();
	testEmptyStatisticsQuery(query, 0);
	// 4 triangles clipped (result of running this test)
	//testBoxStatisticsQuery(query, 8);

	// cannot exactly predict result, only that the counter increases at least by one per visible primitive
	SECTION("testBoxStatisticsQuery") {
		RenderingContext& context = *TestUtils::context.get();
		REQUIRE(query.isValid());
		query.begin(context);
		drawBox(context, box);
		query.end(context);
		REQUIRE(4 <= query.getResult(context));
	}
}

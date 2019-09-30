/*
	This file is part of the Rendering library.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include <catch2/catch.hpp>

#include <Rendering/BufferObject.h>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <random>
#include <vector>

TEST_CASE("BufferObjectTest_test", "[BufferObjectTest]") {
	using namespace Rendering;
	{ // Check prepare() and destroy()
		BufferObject boA;
		BufferObject boB;
		REQUIRE(!boA.isValid());
		REQUIRE(!boB.isValid());
		boA.prepare();
		boB.prepare();
		REQUIRE(boA.isValid());
		REQUIRE(boB.isValid());
		boA.destroy();
		boB.destroy();
		REQUIRE(!boA.isValid());
		REQUIRE(!boB.isValid());
	}
	{ // Check move constructor
		BufferObject boA;
		boA.prepare();
		BufferObject boB(std::move(boA));
		REQUIRE(!boA.isValid());
		REQUIRE(boB.isValid());
	}
	{ // Check move assignment operator
		BufferObject boA;
		BufferObject boB;
		boA.prepare();
		boB.prepare();
		boB = std::move(boA);
		REQUIRE(boA.isValid());
		REQUIRE(boB.isValid());
	}
}

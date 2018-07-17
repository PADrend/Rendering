/*
	This file is part of the Rendering library.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "BufferObjectTest.h"
#include <cppunit/TestAssert.h>
#include <Rendering/Memory/BufferObject.h>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <random>
#include <vector>
CPPUNIT_TEST_SUITE_REGISTRATION(BufferObjectTest);

void BufferObjectTest::test() {
	using namespace Rendering;
	{ // Check prepare() and destroy()
		BufferObject boA;
		BufferObject boB;
		CPPUNIT_ASSERT(!boA.isValid());
		CPPUNIT_ASSERT(!boB.isValid());
		boA.prepare();
		boB.prepare();
		CPPUNIT_ASSERT(boA.isValid());
		CPPUNIT_ASSERT(boB.isValid());
		boA.destroy();
		boB.destroy();
		CPPUNIT_ASSERT(!boA.isValid());
		CPPUNIT_ASSERT(!boB.isValid());
	}
	{ // Check move constructor
		BufferObject boA;
		boA.prepare();
		BufferObject boB(std::move(boA));
		CPPUNIT_ASSERT(!boA.isValid());
		CPPUNIT_ASSERT(boB.isValid());
	}
	{ // Check move assignment operator
		BufferObject boA;
		BufferObject boB;
		boA.prepare();
		boB.prepare();
		boB = std::move(boA);
		CPPUNIT_ASSERT(boA.isValid());
		CPPUNIT_ASSERT(boB.isValid());
	}
}

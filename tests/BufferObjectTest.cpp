/*
	This file is part of the Rendering library.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "TestUtils.h"
#include <catch2/catch.hpp>

#include <Rendering/BufferObject.h>
#include <Rendering/Core/BufferStorage.h>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <random>
#include <vector>

TEST_CASE("BufferObjectTest_test", "[BufferObjectTest]") {
	using namespace Rendering;
	{ // Check create() and destroy()
		BufferObject::Ref boA = BufferObject::create(TestUtils::device);
		BufferObject::Ref boB = BufferObject::create(TestUtils::device);
		REQUIRE(boA);
		REQUIRE(boB);
		REQUIRE(!boA->isValid());
		REQUIRE(!boB->isValid());
		REQUIRE(boA->allocate(128));
		REQUIRE(boB->allocate(256, ResourceUsage::General, MemoryUsage::CpuToGpu, true));
		REQUIRE(boA->isValid());
		REQUIRE(boB->isValid());
		boA->destroy();
		boB->destroy();
		REQUIRE(!boA->isValid());
		REQUIRE(!boB->isValid());
	}
	{ // Check upload & download
		BufferObject::Ref bo = BufferObject::create(TestUtils::device);
		std::vector<uint32_t> data {1,2,3,4,42,666,0xffffffffu,0};
		REQUIRE(bo);
		REQUIRE(bo->allocate(sizeof(uint32_t) * data.size()));
		REQUIRE(bo->isValid());
		REQUIRE(bo->getBuffer()->isMappable());
		REQUIRE(bo->getSize() == sizeof(uint32_t) * data.size());
		bo->upload(data);
		auto data2 = bo->download<uint32_t>(data.size());
		REQUIRE(data.size() == data2.size());
		for(uint32_t i=0; i<data.size(); ++i) {
			REQUIRE(data[i] == data2[i]);
		}
	}
	{ // Check device local
		BufferObject::Ref bo = BufferObject::create(TestUtils::device);
		std::vector<uint32_t> data {1,2,3,4,42,666,0xffffffffu,0};
		REQUIRE(bo);
		REQUIRE(bo->allocate(sizeof(uint32_t) * data.size(), ResourceUsage::General, MemoryUsage::GpuOnly));
		REQUIRE(bo->isValid());
		REQUIRE(!bo->getBuffer()->isMappable());
		REQUIRE(bo->getSize() == sizeof(uint32_t) * data.size());
		bo->upload(data);
		auto data2 = bo->download<uint32_t>(data.size());
		REQUIRE(data.size() == data2.size());
		for(uint32_t i=0; i<data.size(); ++i) {
			REQUIRE(data[i] == data2[i]);
		}
	}
}

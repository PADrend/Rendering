/*
	This file is part of the Rendering library.
	Copyright (C) 2013 Benjamin Eikel <benjamin@eikel.org>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "TestUtils.h"
#include <catch2/catch.hpp>

#include <Rendering/Buffer/BufferObject.h>
#include <Rendering/Buffer/BufferPool.h>
#include <Rendering/Core/BufferStorage.h>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <random>
#include <vector>

using namespace Rendering;

TEST_CASE("BufferObjectTest_testBufferObject", "[BufferObjectTest]") {
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


TEST_CASE("BufferObjectTest_testBufferPool", "[BufferObjectTest]") {
	auto pool = BufferPool::create(TestUtils::device, {
		8, // blockSize
		4, // blocksPerPage
		MemoryUsage::CpuToGpu, // access
		true, // persistent
		ResourceUsage::General, // usage
	});
	REQUIRE(pool);
	
	// 0000

	{
		auto bo1 = pool->allocate(8);
		REQUIRE(bo1);
		REQUIRE(bo1->isValid());
		REQUIRE(bo1->getSize() == 8);
		REQUIRE(bo1->getOffset() == 0);
		REQUIRE(pool->getAllocatedBlockCount() == 1);
		REQUIRE(pool->getAllocatedPageCount() == 1);

		// 1000
		
		auto bo2 = pool->allocate(5); 
		REQUIRE(bo2);
		REQUIRE(bo2->isValid());
		REQUIRE(bo2->getSize() == 8);
		REQUIRE(bo2->getOffset() == 8);
		REQUIRE(bo1->getBuffer() == bo2->getBuffer());
		REQUIRE(pool->getAllocatedBlockCount() == 2);
		REQUIRE(pool->getAllocatedPageCount() == 1);

		// 1100
		
		auto bo3 = pool->allocate(14); 
		REQUIRE(bo3);
		REQUIRE(bo3->isValid());
		REQUIRE(bo3->getSize() == 16);
		REQUIRE(bo3->getOffset() == 16);
		REQUIRE(bo1->getBuffer() == bo3->getBuffer());
		REQUIRE(pool->getAllocatedBlockCount() == 4);
		REQUIRE(pool->getAllocatedPageCount() == 1);

		// 1111

		bo2->destroy();
		REQUIRE(!bo2->isValid());
		REQUIRE(bo2->getSize() == 0);
		REQUIRE(bo2->getOffset() == 0);
		REQUIRE(pool->getAllocatedBlockCount() == 3);
		REQUIRE(pool->getAllocatedPageCount() == 1);

		// 1011
			
		auto bo4 = pool->allocate(17); 
		REQUIRE(bo4);
		REQUIRE(bo4->isValid());
		REQUIRE(bo4->getSize() == 24);
		REQUIRE(bo4->getOffset() == 0);
		REQUIRE(bo1->getBuffer() != bo4->getBuffer());
		auto bo4_buffer = bo4->getBuffer();
		REQUIRE(pool->getAllocatedBlockCount() == 6);
		REQUIRE(pool->getAllocatedPageCount() == 2);

		// 1011-1110

		bo4->destroy();
		REQUIRE(!bo4->isValid());
		REQUIRE(bo4->getSize() == 0);
		REQUIRE(bo4->getOffset() == 0);
		REQUIRE(pool->getAllocatedBlockCount() == 3);
		REQUIRE(pool->getAllocatedPageCount() == 1);

		// 1011
			
		bo2 = pool->allocate(5); 
		REQUIRE(bo2);
		REQUIRE(bo2->isValid());
		REQUIRE(bo2->getSize() == 8);
		REQUIRE(bo2->getOffset() == 8);
		REQUIRE(bo1->getBuffer() == bo2->getBuffer());
		REQUIRE(pool->getAllocatedBlockCount() == 4);
		REQUIRE(pool->getAllocatedPageCount() == 1);

		// 1111
	}
	
	REQUIRE(pool->getAllocatedBlockCount() == 0);
	REQUIRE(pool->getAllocatedPageCount() == 0);
}
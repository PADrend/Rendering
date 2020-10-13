/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "BufferPool.h"
#include "BufferObject.h"

#include "../Core/Device.h"
#include "../Core/BufferStorage.h"

#include <Util/Utils.h>
#include <Util/Macros.h>

#include <algorithm>

namespace Rendering {

//---------------

BufferPool::Ref BufferPool::create(const DeviceRef& device, const Configuration& config) {
	return new BufferPool(device, config);
}

//---------------

BufferPool::~BufferPool() = default;

//---------------

BufferPool::BufferPool(const DeviceRef& device, const Configuration& config) : device(device), config(config) { }

//---------------

BufferObjectRef BufferPool::allocate(size_t size) {
	if(size == 0)
		return nullptr;
	uint32_t count = Util::align(size, config.blockSize)/config.blockSize;
	WARN_AND_RETURN_IF(count > config.blocksPerPage, 
		"Cannot allocate buffer object of size " + std::to_string(config.blockSize*count) + 
		" from pool with maximum page size " + std::to_string(config.blockSize*config.blocksPerPage) + ".", nullptr	
	);
	BufferObjectRef obj = nullptr;
	for(auto& page : pages) {
		if(page.freeBlocks >= count) {
			obj = allocateFromPage(page, count);
			if(obj)
				return obj;
		}
	}

	// allocate new page
	Page page{};
	page.freeBlocks = config.blocksPerPage;
	page.blocks.resize(config.blocksPerPage, false);
	page.buffer = BufferStorage::create(device, {
		config.blocksPerPage*config.blockSize,
		config.access,
		config.persistent,
		config.usage
	});
	WARN_AND_RETURN_IF(!page.buffer, "BufferPool: Failed to allocate new page.", nullptr);
	pages.emplace_back(std::move(page));
	obj = allocateFromPage(pages.back(), count);
	WARN_IF(!obj || !obj->isValid(), "BufferPool: Failed to allocate buffer object.");
	return obj;
}

//---------------

void BufferPool::free(BufferObject* buffer) {
	if(!buffer || !buffer->isValid())
		return;
	
	// find page the buffer was allocated from
	auto it = std::find_if(pages.begin(), pages.end(), [&buffer](const Page& page) {
		return page.buffer == buffer->getBuffer();
	});
	if(it == pages.end())
		return; // no corresponding page found
	auto& page = *it;
	uint32_t index = static_cast<uint32_t>(buffer->getOffset()/config.blockSize);
	uint32_t count = static_cast<uint32_t>(buffer->getSize()/config.blockSize);
	//buffer->destroy(); // TODO: wait until nobody uses the buffer anymore.

	page.freeBlocks += count;
	std::fill(page.blocks.begin() + index, page.blocks.begin() + index + count, false);
	if(page.freeBlocks >= config.blocksPerPage) {
		// release page
		// TODO: might be inefficient. Maybe only release pages if there is another empty one
		pages.erase(it);
	}
}

//---------------

void BufferPool::reset() {
	pages.clear();
}

//---------------

uint32_t BufferPool::getAllocatedBlockCount() const {
	uint32_t count = 0;
	for(auto& page : pages) {
		count += std::count(page.blocks.begin(), page.blocks.end(), true);
	}
	return count;
}

//---------------

BufferObjectRef BufferPool::allocateFromPage(Page& page, uint32_t count) {
	// find free block that is large enough
	uint32_t start = 0;
	uint32_t freeBlocks = 0;
	while(start<=config.blocksPerPage-count && freeBlocks < count) {
		freeBlocks = 0;
		while(freeBlocks<=count && !page.blocks[start+freeBlocks]) {
			++freeBlocks;
		}
		if(freeBlocks < count)
			start+=std::max(freeBlocks,1u);
	}
	if(freeBlocks < count)
		return nullptr;
	std::fill(page.blocks.begin() + start, page.blocks.begin() + start + count, true);
	page.freeBlocks -= count;
	return BufferObject::create(page.buffer, count * config.blockSize, start * config.blockSize, this);
}

//---------------

} /* Rendering */
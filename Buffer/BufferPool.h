/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_BUFFER_BUFFERPOOL_H_
#define RENDERING_BUFFER_BUFFERPOOL_H_

#include "BufferAllocator.h"
#include "../Core/Common.h"

#include <Util/ReferenceCounter.h>
#include <Util/Resources/ResourceAllocator.h>

#include <vector>

namespace Rendering {
class Device;
using DeviceRef = Util::Reference<Device>;
class BufferStorage;
using BufferStorageRef = Util::Reference<BufferStorage>;
class BufferObject;
using BufferObjectRef = Util::Reference<BufferObject>;

class BufferPool : public BufferAllocator {
public:
	using Ref = Util::Reference<BufferPool>;
	struct Configuration {
		size_t blockSize;
		uint32_t blocksPerPage;
		MemoryUsage access = MemoryUsage::CpuToGpu; //! memory access flag
		bool persistent = true; //! if true, the memory of the buffer is persistently mapped to CPU memory 
		ResourceUsage usage = ResourceUsage::General; //! usage flags
	};

	static Ref create(const DeviceRef& device, const Configuration& config);
	~BufferPool();
	BufferPool(BufferPool&& o) = default;
	BufferPool(const BufferPool& o) = delete;

	BufferObjectRef allocate(size_t size) override;
	void free(BufferObject* buffer) override;
	void reset();
	
	uint32_t getAllocatedBlockCount() const;
	uint32_t getAllocatedPageCount() const { return static_cast<uint32_t>(pages.size()); }
private:
	BufferPool(const DeviceRef& device, const Configuration& config);
	const DeviceRef device;
	const Configuration config;

	struct Page {
		BufferStorageRef buffer;
		std::vector<bool> blocks;
		uint32_t freeBlocks;
	};
	std::vector<Page> pages;

	BufferObjectRef allocateFromPage(Page& page, uint32_t count);
};


} /* Rendering */

#endif /* end of include guard: RENDERING_BUFFER_BUFFERPOOL_H_ */
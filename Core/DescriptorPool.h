/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_CORE_DESCRIPTORPOOL_H_
#define RENDERING_CORE_DESCRIPTORPOOL_H_

#include "Common.h"
#include "../Shader/ShaderUtils.h"

#include <Util/ReferenceCounter.h>

#include <unordered_map>
#include <deque>

namespace Rendering {
class Shader;
using ShaderRef = Util::Reference<Shader>;
class DescriptorSet;
using DescriptorSetRef = Util::Reference<DescriptorSet>;

class DescriptorPool : public Util::ReferenceCounter<DescriptorPool> {
public:
	static const uint32_t maxDescriptorCount = 16;

	using Ref = Util::Reference<DescriptorPool>;

	~DescriptorPool();
	DescriptorPool(DescriptorPool&& o) = delete;
	DescriptorPool(const DescriptorPool& o) = delete;

	std::pair<DescriptorSetHandle, DescriptorPoolHandle> request();
	void free(DescriptorSetHandle handle);
	void reset();

	const ShaderResourceLayoutSet& getLayout() const;
	const DescriptorSetLayoutHandle& getLayoutHandle() const { return layoutHandle; }
private:
	friend class Shader;
	explicit DescriptorPool(const ShaderRef& shader, uint32_t set, uint32_t maxDescriptors=maxDescriptorCount);
	bool init();
	DescriptorPoolHandle createPool();

	const ShaderRef shader;
	const uint32_t set;
	const uint32_t maxDescriptors;

	struct PoolEntry {
		PoolEntry(DescriptorPoolHandle&& pool) : pool(std::move(pool)) {}
		DescriptorPoolHandle pool;
		uint32_t allocations;
		std::deque<DescriptorSetHandle> free;
	};
	std::vector<PoolEntry> pools;
	uint32_t currentPoolIndex = 0;
	DescriptorSetLayoutHandle layoutHandle;

};

} /* Rendering */

#endif /* end of include guard: RENDERING_CORE_DESCRIPTORPOOL_H_ */
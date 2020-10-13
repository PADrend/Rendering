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
#include "../State/ShaderLayout.h"

#include <Util/ReferenceCounter.h>
#include <Util/Factory/ObjectPool.h>

#include <array>

namespace Rendering {
class Device;
using DeviceRef = Util::Reference<Device>;
class DescriptorSet;
using DescriptorSetRef = Util::Reference<DescriptorSet>;
class BindingSet;

class DescriptorPool : public Util::ReferenceCounter<DescriptorPool> {
public:
	using Ref = Util::Reference<DescriptorPool>;
	class Configuration {
	public:
		Configuration& setDescriptorCount(ShaderResourceType type, uint32_t count) {
			totalCount -= counts[static_cast<uint32_t>(type)];
			totalCount += count;
			counts[static_cast<uint32_t>(type)] = count;
			return *this;
		}
	private:
		friend class DescriptorPool;
		std::array<uint32_t, static_cast<uint32_t>(ShaderResourceType::ResourceTypeCount)> counts = {};
		uint32_t totalCount = 0;
	};

	static Ref create(const DeviceRef& device, const Configuration& config);

	~DescriptorPool();
	DescriptorPool(DescriptorPool&& o) = delete;
	DescriptorPool(const DescriptorPool& o) = delete;

	DescriptorSetRef requestDescriptorSet(const ShaderResourceLayoutSet& layout, const BindingSet& bindings);
	void reset();

	uint32_t getMaxDescriptorCount(ShaderResourceType type) const { return config.counts[static_cast<uint32_t>(type)]; }

	const DescriptorPoolHandle& getApiHandle() const { return handle; }

	void setDebugName(const std::string& name);
private:
	friend class DescriptorSet;
	explicit DescriptorPool(const DeviceRef& device, const Configuration& config);
	bool init();
	DescriptorSetHandle createDescriptorSet(const DescriptorSetLayoutHandle& layout);
	void updateDescriptorSet(const DescriptorSetRef& descriptorSet, const ShaderResourceLayoutSet& layout, const BindingSet& bindings);
	void free(DescriptorSet* descriptorSet);

	const Util::WeakPointer<Device> device;
	const Configuration config;
	DescriptorPoolHandle handle;

	Util::ObjectPool<DescriptorSetHandle, size_t> pool;
};

//--------------------------------------

class DescriptorSet : public Util::ReferenceCounter<DescriptorSet> {
public:
	using Ref = Util::Reference<DescriptorSet>;
	~DescriptorSet() { pool->free(this); }
	DescriptorSet(DescriptorSet&& o) = default;
	DescriptorSet(const DescriptorSet& o) = delete;
	const DescriptorSetHandle& getApiHandle() const { return handle; }
private:
	friend class DescriptorPool;
	DescriptorSet(const DescriptorPool::Ref& pool, const DescriptorSetHandle& handle, size_t layoutHash) : pool(pool), handle(handle), layoutHash(layoutHash) {}
	const DescriptorPool::Ref pool;
	const DescriptorSetHandle handle;
	size_t layoutHash;
};

} /* Rendering */

#endif /* end of include guard: RENDERING_CORE_DESCRIPTORPOOL_H_ */
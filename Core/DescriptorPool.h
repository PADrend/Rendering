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

#include <vector>
#include <deque>

namespace Rendering {
class Device;
using DeviceRef = Util::Reference<Device>;
class DescriptorSet;
using DescriptorSetRef = Util::Reference<DescriptorSet>;
class DescriptorSetLayout;
using DescriptorSetLayoutRef = Util::Reference<DescriptorSetLayout>;

class DescriptorPool : public Util::ReferenceCounter<DescriptorPool> {
public:
	using Ref = Util::Reference<DescriptorPool>;
	~DescriptorPool() = default;
	DescriptorPool(DescriptorPool&& o) = delete;
	DescriptorPool(const DescriptorPool& o) = delete;

	DescriptorSetRef request();
	void free(DescriptorSet* obj);
	void reset();

	uint32_t getSetIndex() const { return set; }
	const DescriptorSetLayoutRef& getLayout() const { return layout; }
	const DescriptorSetHandle& getDescriptorHandle(uint32_t id) const;
private:
	friend class Shader;
	explicit DescriptorPool(const DeviceRef& device, const ShaderResourceList& resources, uint32_t set);
	bool init();

	const DeviceRef device;
	const uint32_t set = 0;
	DescriptorSetLayoutRef layout;
	std::vector<DescriptorPoolHandle> pools;
	uint32_t poolCounter = 0;
	std::deque<uint32_t> freeDescriptorIds;
	std::deque<DescriptorSetHandle> descriptors;
};

} /* Rendering */

#endif /* end of include guard: RENDERING_CORE_DESCRIPTORPOOL_H_ */
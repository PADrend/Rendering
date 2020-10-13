/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef RENDERING_CORE_DESCRIPTORSETLAYOUT_H_
#define RENDERING_CORE_DESCRIPTORSETLAYOUT_H_

#include "Common.h"
#include "../Shader/ShaderUtils.h"

#include <Util/ReferenceCounter.h>

namespace Rendering {
class Device;
using DeviceRef = Util::Reference<Device>;

class DescriptorSetLayout : public Util::ReferenceCounter<DescriptorSetLayout> {
public:
	using Ref = Util::Reference<DescriptorSetLayout>;
	static Ref create(const DeviceRef& device, const ShaderResourceList& resources);
	~DescriptorSetLayout() = default;
	DescriptorSetLayout(DescriptorSetLayout&& o) = default;
	DescriptorSetLayout(const DescriptorSetLayout& o) = delete;

	const ShaderResourceList& getResources() const { return resources; }
	const DescriptorSetLayoutHandle& getApiHandle() const { return handle; }
	size_t getHash() const { return hash; }
private:
	DescriptorSetLayout(const DeviceRef& device, const ShaderResourceList& resources);
	bool init();
	
	const DeviceRef device;
	const ShaderResourceList resources;
	DescriptorSetLayoutHandle handle;
	size_t hash;
};

} /* Rendering */

#endif /* end of include guard: RENDERING_CORE_DESCRIPTORSETLAYOUT_H_ */
/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "DescriptorPool.h"
#include "DescriptorSet.h"
#include "Device.h"
#include "ResourceCache.h"
#include "../Shader/Shader.h"

#include <vulkan/vulkan.hpp>

#include <numeric>
#include <algorithm>

namespace Rendering {

//-----------------

static const uint32_t maxDescriptorCount = 16;

//-----------------

bool hasBindingPoint(const ShaderResourceType& type);
vk::DescriptorType getVkDescriptorType(const ShaderResourceType& type, bool dynamic);

//-----------------

static DescriptorPoolHandle createPool(const DeviceRef& device, const ShaderResourceLayoutSet& layoutSet) {
	vk::Device vkDevice(device->getApiHandle());

	std::map<vk::DescriptorType, uint32_t> descriptorTypeCount;
	for(const auto& layout : layoutSet.getLayouts()) {
		if(!hasBindingPoint(layout.second.type))
			continue; // Skip resources whitout a binding point

		auto vkType = getVkDescriptorType(layout.second.type, layout.second.dynamic);
		descriptorTypeCount[vkType] += layout.second.elementCount;
	}

	std::vector<vk::DescriptorPoolSize> poolSizes;
	for(auto& it : descriptorTypeCount)
		if(it.second > 0)
			poolSizes.emplace_back(it.first, it.second * maxDescriptorCount);

	return DescriptorPoolHandle::create(vkDevice.createDescriptorPool({
		vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
		maxDescriptorCount,
		static_cast<uint32_t>(poolSizes.size()), poolSizes.data()
	}), vkDevice);
}

//-----------------

DescriptorPool::DescriptorPool(const DeviceRef& device, uint32_t set, const ShaderResourceLayoutSet& layout) : device(device), set(set), layout(layout) { }

//-----------------

DescriptorPool::~DescriptorPool() = default;

//-----------------

bool DescriptorPool::init() {
	layoutHandle = device->getResourceCache()->createDescriptorSetLayout(layout);
	if(!layoutHandle)
		return false;
	reset();
	return true;
}

//-----------------

DescriptorSetHandle DescriptorPool::request() {
	vk::Device vkDevice(device->getApiHandle());
	if(!freeObjects.empty()) {
		// Use existing descriptor
		DescriptorSetHandle obj = freeObjects.front();
		freeObjects.pop_front();
		return obj;
	} else if (poolCounter < maxDescriptorCount) {
		vk::DescriptorPool vkPool(pools.back());
		vk::DescriptorSetLayout vkLayout(layoutHandle);
		
		// Allocate new descriptor set from available pools
		auto vkDescriptor = vkDevice.allocateDescriptorSets({
			vkPool, 1, &vkLayout
		}).front();
		freeObjects.emplace_back(DescriptorSetHandle::create(vkDescriptor, {vkDevice, vkPool}));
		++poolCounter;
		return request();
	} else {
		// Create new pool
		pools.emplace_back(std::move(createPool(device, layout)));
		poolCounter = 0;
		return request();
	}
}

//-----------------

void DescriptorPool::free(DescriptorSetHandle obj) {
	if(obj) {
		freeObjects.emplace_back(obj);
		activeObjects.erase(obj);
	}
}

//-----------------

void DescriptorPool::reset() {
	freeObjects.clear();
	activeObjects.clear();
	pools.clear();
	poolCounter = maxDescriptorCount;
}

//-----------------

} /* Rendering */
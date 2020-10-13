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

vk::DescriptorType getVkDescriptorType(const ShaderResourceType& type, bool dynamic);

//-----------------

DescriptorPool::DescriptorPool(const ShaderRef& shader, uint32_t set, uint32_t maxDescriptors) : shader(shader), set(set), maxDescriptors(maxDescriptors) { }

//-----------------

DescriptorPool::~DescriptorPool() = default;

//-----------------

bool DescriptorPool::init() {
	auto& device = shader->getDevice();
	const auto& layout = shader->getLayout().getLayoutSet(set);
	layoutHandle = device->getResourceCache()->createDescriptorSetLayout(layout);
	if(!layoutHandle)
		return false;
	reset();
	return true;
}

//-----------------

std::pair<DescriptorSetHandle, DescriptorPoolHandle> DescriptorPool::request() {
	auto& device = shader->getDevice();
	vk::Device vkDevice(device->getApiHandle());

	// find a free pool
	// TODO: might be too slow? Maybe use a map instead.
	uint32_t maxCount = maxDescriptors;
	const auto& it = std::find_if(pools.begin() + currentPoolIndex, pools.end(), [&maxCount](const PoolEntry& entry) {
		return entry.allocations < maxCount || !entry.free.empty();
	});
	
	if(it != pools.end()) {
		if(!it->free.empty()) {
			// Use free existing descriptor set
			DescriptorSetHandle obj = it->free.front();
			it->free.pop_front();
			return {obj, it->pool};
		} else {
			// Allocate new descriptor set from pool
			vk::DescriptorPool vkPool(it->pool);
			vk::DescriptorSetLayout vkLayout(layoutHandle);
			
			// Allocate new descriptor set from available pools
			auto obj = vkDevice.allocateDescriptorSets({
				vkPool, 1, &vkLayout
			}).front();
			it->allocations++;
			return {DescriptorSetHandle::create(obj, {vkDevice, it->pool}), it->pool};
		}
	} else {
		// No free pool found: Create a new one.
		pools.emplace_back(std::move(createPool()));
		currentPoolIndex = static_cast<uint32_t>(pools.size()-1);
		return request();
	}
}

//-----------------

void DescriptorPool::free(DescriptorSetHandle obj) {
	if(!obj)
		return;
	// find the pool the descriptor set belongs to
	const auto& it = std::find_if(pools.begin(), pools.end(), [&obj](const PoolEntry& entry) {
		DescriptorSetParent parent = obj;
		return static_cast<VkDescriptorPool>(entry.pool) == parent.second;
	});
	if(it != pools.end()) {
		it->free.emplace_back(obj);
		currentPoolIndex = it - pools.begin();
	}
}

//-----------------

void DescriptorPool::reset() {
	pools.clear();
	currentPoolIndex = 0;
}

//-----------------

const ShaderResourceLayoutSet& DescriptorPool::getLayout() const {
	return shader->getLayout().getLayoutSet(set);
}

//-----------------

DescriptorPoolHandle DescriptorPool::createPool() {
	auto& device = shader->getDevice();
	const auto& layoutSet = shader->getLayout().getLayoutSet(set);
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
			poolSizes.emplace_back(it.first, it.second * maxDescriptors);

	return DescriptorPoolHandle::create(vkDevice.createDescriptorPool({
		vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
		maxDescriptors,
		static_cast<uint32_t>(poolSizes.size()), poolSizes.data()
	}), vkDevice);
}

//-----------------

} /* Rendering */
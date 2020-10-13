/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "DescriptorPool.h"
#include "DescriptorSet.h"
#include "DescriptorSetLayout.h"
#include "Device.h"
#include "../Shader/Shader.h"

#include <vulkan/vulkan.hpp>

#include <numeric>

namespace Rendering {

//-----------------

static const uint32_t maxDescriptorCount = 16;

//-----------------

bool hasBindingPoint(const ShaderResourceType& type);
vk::DescriptorType getVkDescriptorType(const ShaderResourceType& type, bool dynamic);

//-----------------

static DescriptorPoolHandle createPool(const DeviceRef& device, const ShaderResourceList& resources) {
	vk::Device vkDevice(device->getApiHandle());

	std::map<vk::DescriptorType, uint32_t> descriptorTypeCount;
	for(const auto& resource : resources) {
		if(!hasBindingPoint(resource.type))
			continue; // Skip resources whitout a binding point

		auto vkType = getVkDescriptorType(resource.type, resource.dynamic);
		descriptorTypeCount[vkType] += resource.array_size;
	}

	std::vector<vk::DescriptorPoolSize> poolSizes;
	for(auto& it : descriptorTypeCount)
		if(it.second > 0)
			poolSizes.emplace_back(it.first, it.second * maxDescriptorCount);

	return {vkDevice.createDescriptorPool({
		vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
		maxDescriptorCount,
		static_cast<uint32_t>(poolSizes.size()), poolSizes.data()
	}), vkDevice};
}

//-----------------

DescriptorPool::DescriptorPool(const DeviceRef& device, uint32_t set) : device(device), set(set) { }

//-----------------

DescriptorPool::~DescriptorPool() = default;

//-----------------

bool DescriptorPool::init(const ShaderResourceList& resources) {
	layout = DescriptorSetLayout::create(device, resources);
	if(!layout)
		return false;
	reset();
	return true;
}

//-----------------

DescriptorSetRef DescriptorPool::request() {
	vk::Device vkDevice(device->getApiHandle());
	if(!freeDescriptorIds.empty()) {
		// Use existing descriptor
		DescriptorSetRef obj = new DescriptorSet(this, freeDescriptorIds.front());
		freeDescriptorIds.pop_front();
		return obj;
	} else if (poolCounter < maxDescriptorCount) {
		vk::DescriptorPool vkPool(pools.back());
		vk::DescriptorSetLayout vkLayout(layout->getApiHandle());
		
		// Allocate new descriptor set from available pools
		auto vkDescriptor = vkDevice.allocateDescriptorSets({
			vkPool, 1, &vkLayout
		}).front();
		freeDescriptorIds.emplace_back(descriptors.size());
		descriptors.emplace_back(
			static_cast<VkDescriptorSet>(vkDescriptor), 
			static_cast<VkDevice>(vkDevice), 
			static_cast<VkDescriptorPool>(vkPool)
		);
		++poolCounter;
		return request();
	} else {
		// Create new pool
		pools.emplace_back(std::move(createPool(device, layout->getResources())));
		poolCounter = 0;
		return request();
	}
}

//-----------------

void DescriptorPool::free(DescriptorSet* obj) {
	if(obj && obj->descriptorId < std::numeric_limits<uint32_t>::max()) {
		freeDescriptorIds.emplace_back(obj->descriptorId);
		obj->descriptorId = std::numeric_limits<uint32_t>::max();
	}
}

//-----------------

void DescriptorPool::reset() {
	freeDescriptorIds.clear();
	descriptors.clear();
	pools.clear();
	poolCounter = maxDescriptorCount;
}

//-----------------

const DescriptorSetHandle& DescriptorPool::getDescriptorHandle(uint32_t id) const {
	if(id < descriptors.size())
		return nullptr;
	return descriptors[id];
}

//-----------------

} /* Rendering */
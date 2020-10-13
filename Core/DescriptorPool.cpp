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

DescriptorPool::Ref DescriptorPool::create(const DeviceRef& device, const Configuration& config) {
	Ref pool = new DescriptorPool(device, config);
	if(!pool->init())
		return nullptr;
	return pool;
}

//-----------------

DescriptorPool::DescriptorPool(const DeviceRef& device, const Configuration& config) : device(device), config(config) { }

//-----------------

DescriptorPool::~DescriptorPool() = default;

//-----------------

bool DescriptorPool::init() {
	vk::Device vkDevice(device->getApiHandle());

	std::vector<vk::DescriptorPoolSize> poolSizes;
	for(uint32_t t = 0; t < config.counts.size(); ++t) {
		if(config.counts[t] > 0)
			poolSizes.emplace_back(getVkDescriptorType(static_cast<ShaderResourceType>(t), false), config.counts[t]);
	}

	handle = DescriptorPoolHandle::create(vkDevice.createDescriptorPool({
		vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
		config.totalCount,
		static_cast<uint32_t>(poolSizes.size()), poolSizes.data()
	}), vkDevice);
	return handle.isNotNull();
}

//-----------------

DescriptorSetLayoutHandle DescriptorPool::getLayoutHandle(const ShaderResourceLayoutSet& layout) {
	return device->getResourceCache()->createDescriptorSetLayout(layout);
}

//-----------------

DescriptorSetHandle DescriptorPool::requestDescriptorSet(const ShaderResourceLayoutSet& layout) {
	size_t layoutHash = std::hash<ShaderResourceLayoutSet>{}(layout);
	if(!pool.hasType(layoutHash)) {
		auto layoutHandle = getLayoutHandle(layout);
		pool.registerType(layoutHash, std::bind(&DescriptorPool::createDescriptorSet, this, layoutHandle));
	}
	return pool.create(layoutHash);
}

//-----------------

void DescriptorPool::free(DescriptorSetHandle obj, size_t layoutHash) {
	if(!obj)
		return;
	pool.free(layoutHash, obj);
}

//-----------------

void DescriptorPool::reset() {
	pool.reset();
	vk::Device vkDevice(handle);
	vkDevice.resetDescriptorPool(static_cast<vk::DescriptorPool>(handle));
}

//-----------------

DescriptorSetHandle DescriptorPool::createDescriptorSet(const DescriptorSetLayoutHandle& layout) {
	vk::Device vkDevice(device->getApiHandle());
	vk::DescriptorPool vkPool(handle);
	vk::DescriptorSetLayout vkLayout(layout);
	
	// Allocate new descriptor set from pool
	auto obj = vkDevice.allocateDescriptorSets({
		vkPool, 1, &vkLayout
	}).front();
	return DescriptorSetHandle::create(obj, {vkDevice, handle});
}

//-----------------

} /* Rendering */
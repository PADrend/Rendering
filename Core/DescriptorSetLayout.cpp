/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "DescriptorSetLayout.h"
#include "Device.h"

#include <Util/Utils.h>

#include <vulkan/vulkan.hpp>

namespace Rendering {


//-----------------

vk::ShaderStageFlags getVkStageFlags(const ShaderStage& stages);
vk::DescriptorType getVkDescriptorType(const ShaderResourceType& type, bool dynamic);
bool hasBindingPoint(const ShaderResourceType& type);

//---------------

DescriptorSetLayout::Ref DescriptorSetLayout::create(const DeviceRef& device, const ShaderResourceList& resources) {
	Ref obj = new DescriptorSetLayout(device, resources);
	if(!obj->init()) {
		return nullptr;
	}
	return obj;
}
//---------------

DescriptorSetLayout::DescriptorSetLayout(const DeviceRef& device, const ShaderResourceList& resources) : device(device), resources(resources), hash(0) {
	for(auto& r : resources)
		Util::hash_combine(hash, r);
}

//---------------

DescriptorSetLayout::~DescriptorSetLayout() = default;

//---------------

bool DescriptorSetLayout::init() {
	
	vk::Device vkDevice(device->getApiHandle());
	std::vector<vk::DescriptorSetLayoutBinding> bindings;
	for(const auto& resource : resources) {
		if(!hasBindingPoint(resource.type))
			continue; // Skip resources whitout a binding point

		vk::DescriptorSetLayoutBinding binding{};
		binding.binding = resource.binding;
		binding.descriptorCount = resource.array_size;
		binding.descriptorType = getVkDescriptorType(resource.type, resource.dynamic);
		binding.stageFlags = getVkStageFlags(resource.stages);

		bindings.emplace_back(binding);
	}

	handle = DescriptorSetLayoutHandle::create(vkDevice.createDescriptorSetLayout({{}, static_cast<uint32_t>(bindings.size()), bindings.data()}), vkDevice);
	return handle.isNotNull();
}

//---------------

} /* Rendering */
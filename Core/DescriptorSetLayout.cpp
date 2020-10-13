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

static inline vk::ShaderStageFlags getVkStageFlags(const ShaderStage& stages) {
	vk::ShaderStageFlags flags;
	if((stages & ShaderStage::Vertex) == ShaderStage::Vertex) flags |= vk::ShaderStageFlagBits::eVertex;
	if((stages & ShaderStage::TessellationControl) == ShaderStage::TessellationControl) flags |= vk::ShaderStageFlagBits::eTessellationControl;
	if((stages & ShaderStage::TessellationEvaluation) == ShaderStage::TessellationEvaluation) flags |= vk::ShaderStageFlagBits::eTessellationEvaluation;
	if((stages & ShaderStage::Geometry) == ShaderStage::Geometry) flags |= vk::ShaderStageFlagBits::eGeometry;
	if((stages & ShaderStage::Fragment) == ShaderStage::Fragment) flags |= vk::ShaderStageFlagBits::eFragment;
	if((stages & ShaderStage::Compute) == ShaderStage::Compute) flags |= vk::ShaderStageFlagBits::eCompute;
	return flags;
}

//-----------------

static inline vk::DescriptorType getVkDescriptorType(const ShaderResourceType& type, bool dynamic) {
	switch (type) {
		case ShaderResourceType::InputAttachment: return vk::DescriptorType::eInputAttachment;
		case ShaderResourceType::Image: return vk::DescriptorType::eSampledImage;
		case ShaderResourceType::ImageSampler: return vk::DescriptorType::eCombinedImageSampler;
		case ShaderResourceType::ImageStorage: return vk::DescriptorType::eStorageImage;
		case ShaderResourceType::Sampler: return vk::DescriptorType::eSampler;
		case ShaderResourceType::BufferUniform: return dynamic ? vk::DescriptorType::eUniformBufferDynamic : vk::DescriptorType::eUniformBuffer;
		case ShaderResourceType::BufferStorage: return dynamic ? vk::DescriptorType::eStorageBufferDynamic : vk::DescriptorType::eStorageBuffer;
		default: return {};
	}
}

//-----------------

static inline bool hasBindingPoint(const ShaderResourceType& type) {
	switch (type) {
		case ShaderResourceType::Input:
		case ShaderResourceType::Output:
		case ShaderResourceType::PushConstant:
		case ShaderResourceType::SpecializationConstant:
		case ShaderResourceType::ResourceTypeCount:
			return false;
		default: 
			return true;
	}
}

//---------------

DescriptorSetLayout::Ref DescriptorSetLayout::create(const DeviceRef& device, const ShaderResourceList& resources) {
	Ref obj = new DescriptorSetLayout(device, resources);
	if(!obj->init()) {
		return nullptr;
	}
	return obj;
}

//---------------

DescriptorSetLayout::~DescriptorSetLayout() = default;

//---------------

DescriptorSetLayout::DescriptorSetLayout(const DeviceRef& device, const ShaderResourceList& resources) : device(device), resources(resources), hash(0) {
	for(auto& r : resources)
		Util::hash_combine(hash, r);
}

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

	handle = std::move(DescriptorSetLayoutHandle(vkDevice.createDescriptorSetLayout({{}, static_cast<uint32_t>(bindings.size()), bindings.data()}), vkDevice));
	return handle;
}

//---------------

} /* Rendering */
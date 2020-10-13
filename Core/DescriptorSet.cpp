/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "DescriptorSet.h"
#include "DescriptorPool.h"
#include "Device.h"
#include "Sampler.h"
#include "ImageView.h"
#include "BufferStorage.h"
#include "../Texture/Texture.h"
#include "../BufferObject.h"

#include <Util/Macros.h>

#include <vulkan/vulkan.hpp>
#include <numeric>

namespace Rendering {


//-----------------

vk::ShaderStageFlags getVkStageFlags(const ShaderStage& stages);
vk::DescriptorType getVkDescriptorType(const ShaderResourceType& type, bool dynamic);
bool hasBindingPoint(const ShaderResourceType& type);

//-----------------

inline static ResourceUsage getResourceUsage(const ShaderResourceType& type) {
	switch (type) {
		case ShaderResourceType::Image:
		case ShaderResourceType::ImageSampler:
		case ShaderResourceType::Sampler:
		case ShaderResourceType::BufferUniform:
			return ResourceUsage::ShaderResource;
		case ShaderResourceType::ImageStorage:
		case ShaderResourceType::BufferStorage: 
			return ResourceUsage::ShaderWrite;
		default: return ResourceUsage::General;
	}
}

//-----------------

inline static vk::ImageLayout getVkImageLayout(const ShaderResourceType& type) {
	switch (type) {
		case ShaderResourceType::InputAttachment:
			return vk::ImageLayout::eColorAttachmentOptimal;
		case ShaderResourceType::Image:
		case ShaderResourceType::ImageSampler:
		case ShaderResourceType::Sampler:
		case ShaderResourceType::BufferUniform:
			return vk::ImageLayout::eShaderReadOnlyOptimal;
		default: return vk::ImageLayout::eGeneral;
	}
}

//---------------

ApiBaseHandle::Ref createDescriptorSetLayoutHandle(Device* device, const ShaderResourceLayoutSet& layoutSet) {
	vk::Device vkDevice(device->getApiHandle());
	std::vector<vk::DescriptorSetLayoutBinding> bindings;
	for(const auto& it : layoutSet.getLayouts()) {
		auto layout = it.second;
		if(!hasBindingPoint(layout.type))
			continue; // Skip resources whitout a binding point

		vk::DescriptorSetLayoutBinding binding{};
		binding.binding = it.first;
		binding.descriptorCount = layout.elementCount;
		binding.descriptorType = getVkDescriptorType(layout.type, layout.dynamic);
		binding.stageFlags = getVkStageFlags(layout.stages);

		bindings.emplace_back(binding);
	}

	return DescriptorSetLayoutHandle::create(vkDevice.createDescriptorSetLayout({{}, static_cast<uint32_t>(bindings.size()), bindings.data()}), vkDevice).get();
}

//---------------

ApiBaseHandle::Ref createPipelineLayoutHandle(Device* device, const ShaderLayout& layout) {
	vk::Device vkDevice(device->getApiHandle());

	for(auto& set : layout.getLayoutSets()) {

	}
	// Separate resources by set index
	std::vector<vk::PushConstantRange> pushConstantRanges;
	/*for(auto& res : resources) {
		setResources[res.second.set].emplace_back(res.second);
		if(res.second.layout.type == ShaderResourceType::PushConstant) {
			pushConstantRanges.emplace_back(getVkStageFlags(res.second.layout.stages), res.second.offset, res.second.size);
		}
	}*/

	std::vector<vk::DescriptorSetLayout> layouts;
	/*for(auto& res : descriptorPools)
		layouts.emplace_back(res.second->getLayout()->getApiHandle());*/

	return PipelineLayoutHandle::create(vkDevice.createPipelineLayout({{},
		static_cast<uint32_t>(layouts.size()), layouts.data(),
		static_cast<uint32_t>(pushConstantRanges.size()), pushConstantRanges.data(),
	}), vkDevice).get();
}

//---------------

DescriptorSet::Ref DescriptorSet::create(const DescriptorPoolRef& pool, const BindingSet& bindings) {
	Ref obj = new DescriptorSet(pool);
	if(!obj->init())
		return nullptr;
	obj->update(bindings);
	return obj;
}

//---------------

DescriptorSet::DescriptorSet(const DescriptorPoolRef& pool) : pool(pool) { }

//---------------

DescriptorSet::~DescriptorSet() {
	pool->free(handle);
}

//---------------

bool DescriptorSet::init() {
	if(!pool)
		return false;
	handle = pool->request();
	layoutHandle = pool->getLayoutHandle();
	return handle.isNotNull();
}

//---------------

bool DescriptorSet::update(const BindingSet& bindings) {
	auto layout = pool->getLayout();
	vk::Device vkDevice(layoutHandle);
	vk::DescriptorSet vkDescriptorSet(handle);
	
	std::vector<vk::WriteDescriptorSet> writes;
	for(auto& bIt : bindings.getBindings()) {
		auto& binding = bIt.second;
		if(!layout.hasLayout(bIt.first))
			continue;
		auto descriptor = layout.getLayout(bIt.first);
		
		auto usage = getResourceUsage(descriptor.type);
		auto vkImageLayout = getVkImageLayout(descriptor.type);

		std::vector<vk::DescriptorImageInfo> imageBindings;
		std::vector<vk::DescriptorBufferInfo> bufferBindings;
		std::vector<vk::BufferView> texelBufferViews; // TODO

		for(auto& tex : binding.getTextures()) {
			if(tex && tex->isValid()) {
				imageBindings.emplace_back(
					static_cast<vk::Sampler>(tex->getSampler()->getApiHandle()), 
					static_cast<vk::ImageView>(tex->getImageView()->getApiHandle()), 
					vkImageLayout);
			} else {
				WARN("Empty texture binding.");
				imageBindings.emplace_back(nullptr, nullptr, vkImageLayout);
			}
		}

		for(auto& view : binding.getInputImages()) {
			if(view && view->getApiHandle()) {
				imageBindings.emplace_back(nullptr, static_cast<vk::ImageView>(view->getApiHandle()), vkImageLayout);
			} else {
				WARN("Empty texture binding.");
				imageBindings.emplace_back(nullptr, nullptr, vkImageLayout);
			}
		}

		for(auto& buffer : binding.getBuffers()) {
			if(buffer && buffer->isValid()) {
				auto b = buffer->getBuffer();
				bufferBindings.emplace_back(static_cast<vk::Buffer>(b->getApiHandle()), 0, b->getSize());
			} else {
				WARN("Empty texture binding.");
				bufferBindings.emplace_back(nullptr, 0, 0);
			}
		}

		// TODO: handle unbound array elements
		uint32_t count = static_cast<uint32_t>(std::max(imageBindings.size(), std::max(bufferBindings.size(), texelBufferViews.size())));
		writes.emplace_back(
			vkDescriptorSet, bIt.first, 
			0, count, getVkDescriptorType(descriptor.type, descriptor.dynamic), 
			imageBindings.data(), bufferBindings.data(), texelBufferViews.data()
		);
	}
	vkDevice.updateDescriptorSets(writes, {});
	return true;
}

//---------------

const DescriptorSetHandle& DescriptorSet::getApiHandle() const {
	return handle;
}

//---------------

} /* Rendering */
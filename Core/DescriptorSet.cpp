/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "DescriptorSet.h"
#include "DescriptorPool.h"
#include "DescriptorSetLayout.h"
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

vk::DescriptorType getVkDescriptorType(const ShaderResourceType& type, bool dynamic);

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

DescriptorSet::Ref DescriptorSet::request(const DescriptorPoolRef& pool, const BindingSet& bindings) {
	Ref obj = pool->request();
	if(!obj)
		return nullptr;
	obj->update(bindings);
	return obj;
}

//---------------

DescriptorSet::DescriptorSet(const DescriptorPoolRef& pool, uint32_t descriptorId) : pool(pool), descriptorId(descriptorId) { }

//---------------

DescriptorSet::~DescriptorSet() {
	pool->free(this);
	descriptorId = std::numeric_limits<uint32_t>::max();
}

//---------------

bool DescriptorSet::update(const BindingSet& bindings) {
	auto layout = pool->getLayout();
	vk::Device vkDevice(layout->getApiHandle());
	vk::DescriptorSet vkDescriptorSet(pool->getDescriptorHandle(descriptorId));
	WARN_AND_RETURN_IF(!vkDescriptorSet, "Descriptor set is invalid or has been freed.", false);
	
	std::vector<vk::WriteDescriptorSet> writes;
	for(auto& bIt : bindings.getBindings()) {
		auto& binding = bIt.second;
		auto descriptor = std::find_if(layout->getResources().begin(), layout->getResources().end(), [&](const ShaderResource& res) {
			return bIt.first == res.binding;
		});
		if(descriptor == layout->getResources().end())
			continue;
		auto usage = getResourceUsage(descriptor->type);
		auto vkImageLayout = getVkImageLayout(descriptor->type);

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
			0, count, getVkDescriptorType(descriptor->type, descriptor->dynamic), 
			imageBindings.data(), bufferBindings.data(), texelBufferViews.data()
		);
	}
	vkDevice.updateDescriptorSets(writes, {});
	return true;
}

//---------------

const DescriptorSetHandle& DescriptorSet::getApiHandle() const {
	return pool->getDescriptorHandle(descriptorId);
}

//---------------

} /* Rendering */
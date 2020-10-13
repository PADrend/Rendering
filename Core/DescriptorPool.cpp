/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "DescriptorPool.h"
#include "Device.h"
#include "ResourceCache.h"
#include "Sampler.h"
#include "ImageView.h"
#include "BufferStorage.h"
#include "../BufferObject.h"
#include "../Shader/Shader.h"
#include "../State/BindingState.h"
#include "../Texture/Texture.h"

#include <Util/Macros.h>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#include <numeric>
#include <algorithm>

namespace Rendering {

//-----------------

vk::DescriptorType getVkDescriptorType(const ShaderResourceType& type, bool dynamic);
vk::ImageLayout getVkImageLayout(const ResourceUsage& usage);

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
		ShaderResourceType type = static_cast<ShaderResourceType>(t);
		if(config.counts[t] > 0)
			poolSizes.emplace_back(getVkDescriptorType(type, type == ShaderResourceType::BufferUniform), config.counts[t]);
	}

	handle = DescriptorPoolHandle::create(vkDevice.createDescriptorPool({
		vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
		config.totalCount,
		static_cast<uint32_t>(poolSizes.size()), poolSizes.data()
	}), vkDevice);
	return handle.isNotNull();
}


//-----------------

DescriptorSetRef DescriptorPool::requestDescriptorSet(const ShaderResourceLayoutSet& layout, const BindingSet& bindings) {
	size_t layoutHash = std::hash<ShaderResourceLayoutSet>{}(layout);
	if(!pool.hasType(layoutHash)) {
		auto layoutHandle = device->getResourceCache()->createDescriptorSetLayout(layout);
		pool.registerType(layoutHash, std::bind(&DescriptorPool::createDescriptorSet, this, layoutHandle));
	}
	
	auto setHandle = pool.create(layoutHash);
	if(!setHandle)
		return nullptr;
	
	DescriptorSet::Ref set = new DescriptorSet({this}, setHandle, layoutHash);
	updateDescriptorSet(set, layout, bindings);
	return set;
}

//-----------------

void DescriptorPool::free(DescriptorSet* descriptorSet) {
	if(!descriptorSet || !descriptorSet->handle)
		return;
	pool.free(descriptorSet->layoutHash, descriptorSet->handle);
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

void DescriptorPool::updateDescriptorSet(const DescriptorSetRef& descriptorSet, const ShaderResourceLayoutSet& layout, const BindingSet& bindings) {
	vk::Device vkDevice(device->getApiHandle());
	vk::DescriptorSet vkDescriptorSet(descriptorSet->getApiHandle());
	descriptorSet->dynamicOffsets.clear();
	
	std::vector<vk::WriteDescriptorSet> writes;

	// need to keep data alive during creation
	std::vector<std::vector<vk::DescriptorImageInfo>> imageBindings;
	std::vector<std::vector<vk::DescriptorBufferInfo>> bufferBindings;
	
	for(auto& bIt : bindings.getBindings()) {
		auto& binding = bIt.second;
		if(!layout.hasLayout(bIt.first))
			continue;
		auto descriptor = layout.getLayout(bIt.first);
		
		auto usage = getResourceUsage(descriptor.type);
		imageBindings.emplace_back();
		bufferBindings.emplace_back();

		for(auto& tex : binding.getTextures()) {
			auto vkImageLayout = getVkImageLayout(tex->getLastUsage());
			if(tex && tex->isValid()) {
				imageBindings.back().emplace_back(
					static_cast<vk::Sampler>(tex->getSampler()->getApiHandle()), 
					static_cast<vk::ImageView>(tex->getImageView()->getApiHandle()), 
					vkImageLayout);
			} else {
				WARN("Empty texture binding.");
				imageBindings.back().emplace_back(nullptr, nullptr, vkImageLayout);
			}
		}

		/*for(auto& view : binding.getInputImages()) {
			if(view && view->getApiHandle()) {
				imageBindings.back().emplace_back(nullptr, static_cast<vk::ImageView>(view->getApiHandle()), vkImageLayout);
			} else {
				WARN("Empty input image binding.");
				imageBindings.back().emplace_back(nullptr, nullptr, vkImageLayout);
			}
		}*/

		for(auto& buffer : binding.getBuffers()) {
			if(buffer && buffer->isValid()) {
				auto b = buffer->getBuffer();
				size_t offset = descriptor.dynamic ? 0 : buffer->getOffset();
				bufferBindings.back().emplace_back(static_cast<vk::Buffer>(b->getApiHandle()), offset, buffer->getSize());
				if(descriptor.dynamic)
					descriptorSet->dynamicOffsets.emplace_back(static_cast<uint32_t>(buffer->getOffset()));
			} else {
				WARN("Empty buffer binding.");
				bufferBindings.back().emplace_back(nullptr, 0, 0);
			}
		}

		// TODO: handle unbound array elements
		uint32_t count = std::max<uint32_t>(imageBindings.back().size(), bufferBindings.back().size());
		writes.emplace_back(
			vkDescriptorSet, bIt.first, 
			0, count, getVkDescriptorType(descriptor.type, descriptor.dynamic), 
			imageBindings.back().data(), bufferBindings.back().data(), nullptr
		);
	}
	vkDevice.updateDescriptorSets(writes, {});
}

//-------------

void DescriptorPool::setDebugName(const std::string& name) {
	if(!device->getConfig().debugMode)
		return;
	vk::Device vkDevice(device->getApiHandle());
	vkDevice.setDebugUtilsObjectNameEXT({ vk::DescriptorPool::objectType, handle, name.c_str() });
}

//-----------------

} /* Rendering */
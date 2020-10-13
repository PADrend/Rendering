/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "Sampler.h"
#include "Device.h"

#include <Util/Macros.h>

#include <vulkan/vulkan.hpp>

namespace Rendering {

//---------------

static vk::Filter getVkFilter(ImageFilter filter) {
	switch(filter) {
		case Nearest: return vk::Filter::eNearest;
		case Linear: return vk::Filter::eLinear;
		default: return {};
	}
};

//---------------

static vk::SamplerMipmapMode getVkMipmapMode(ImageFilter filter) {
	switch(filter) {
		case Nearest: return vk::SamplerMipmapMode::eNearest;
		case Linear: return vk::SamplerMipmapMode::eLinear;
		default: return {};
	}
};

//---------------

static vk::SamplerAddressMode getVkAddressMode(ImageAddressMode filter) {
	switch(filter) {
		case Repeat: return vk::SamplerAddressMode::eRepeat;
		case MirroredRepeat: return vk::SamplerAddressMode::eMirroredRepeat;
		case ClampToEdge: return vk::SamplerAddressMode::eClampToEdge;
		case ClampToBorder: return vk::SamplerAddressMode::eClampToBorder;
		default: return {};
	}
};

//---------------

static vk::CompareOp getVkCompareOp(const ComparisonFunc& op) {
	switch(op) {
		case ComparisonFunc::Never: return vk::CompareOp::eNever;
		case ComparisonFunc::Less: return vk::CompareOp::eLess;
		case ComparisonFunc::Equal: return vk::CompareOp::eEqual;
		case ComparisonFunc::LessOrEqual: return vk::CompareOp::eLessOrEqual;
		case ComparisonFunc::Greater: return vk::CompareOp::eGreater;
		case ComparisonFunc::NotEqual: return vk::CompareOp::eNotEqual;
		case ComparisonFunc::GreaterOrEqual: return vk::CompareOp::eGreaterOrEqual;
		case ComparisonFunc::Always: return vk::CompareOp::eAlways;
	}
	return vk::CompareOp::eNever;
}

//---------------

Sampler::Ref Sampler::create(const DeviceRef& device, const Configuration& config) {
	Ref obj = new Sampler(device, config);
	if(!obj->init()) {
		WARN("Failed to create Sampler.");
		return nullptr;
	}
	return obj;
}

//---------------

Sampler::Sampler(const DeviceRef& device, const Configuration& config) : device(device), config(config) { }

//---------------

bool Sampler::init() {
	vk::Device vkDevice(device->getApiHandle());
	handle = std::move(SamplerHandle(vkDevice.createSampler({{},
		getVkFilter(config.magFilter), getVkFilter(config.minFilter), getVkMipmapMode(config.mipmapMode),
		getVkAddressMode(config.addressModeU), getVkAddressMode(config.addressModeV), getVkAddressMode(config.addressModeW),
		config.mipLodBias,
		config.maxAnisotropy > 1, static_cast<float>(config.maxAnisotropy),
		config.compareOp != ComparisonFunc::Disabled, getVkCompareOp(config.compareOp),
		config.minLod, config.maxLod,
		vk::BorderColor::eFloatTransparentBlack, false
	}), vkDevice));
	return handle;
}

//---------------

} /* Rendering */
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

vk::Filter getVkFilter(const ImageFilter& filter);
vk::SamplerMipmapMode getVkMipmapMode(const ImageFilter& filter);
vk::SamplerAddressMode getVkAddressMode(const ImageAddressMode& filter);
vk::CompareOp getVkCompareOp(const ComparisonFunc& op);

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
	handle = SamplerHandle::create(vkDevice.createSampler({{},
		getVkFilter(config.magFilter), getVkFilter(config.minFilter), getVkMipmapMode(config.mipmapMode),
		getVkAddressMode(config.addressModeU), getVkAddressMode(config.addressModeV), getVkAddressMode(config.addressModeW),
		config.mipLodBias,
		config.maxAnisotropy > 1, static_cast<float>(config.maxAnisotropy),
		config.compareOp != ComparisonFunc::Disabled, getVkCompareOp(config.compareOp),
		config.minLod, config.maxLod,
		vk::BorderColor::eFloatTransparentBlack, false
	}), vkDevice);
	return handle.isNotNull();
}

//---------------

} /* Rendering */
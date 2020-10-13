/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef RENDERING_CORE_INTERNAL_VKUTILS_H_
#define RENDERING_CORE_INTERNAL_VKUTILS_H_

#include "../Common.h"
#include "../ImageStorage.h"
#include "../ImageView.h"
#include "../../State/ShaderLayout.h"
#include <Util/Resources/ResourceFormat.h>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

namespace Rendering {

vk::Format getVkFormat(const InternalFormat& format);

vk::AccessFlags getVkAccessMask(const ResourceUsage& usage);

vk::Filter getVkFilter(const ImageFilter& filter);

vk::SamplerMipmapMode getVkMipmapMode(const ImageFilter& filter);

vk::SamplerAddressMode getVkAddressMode(const ImageAddressMode& filter);

vk::CompareOp getVkCompareOp(const ComparisonFunc& op);

vk::ImageLayout getVkImageLayout(const ResourceUsage& usage);

vk::PipelineStageFlags getVkPipelineStageMask(const ResourceUsage& usage, bool src);

vk::BufferUsageFlags getVkBufferUsage(const ResourceUsage& usage);

vk::ImageUsageFlags getVkImageUsage(const InternalFormat& format, const ResourceUsage& usage);

vk::DescriptorType getVkDescriptorType(const ShaderResourceType& type, bool dynamic);

vk::ShaderStageFlags getVkStageFlags(const ShaderStage& stages);

void transferImageLayout(CommandBufferHandle cmd, const ImageView::Ref& view, ResourceUsage newUsage);

void transferImageLayout(CommandBufferHandle cmd, const ImageStorage::Ref& image, ResourceUsage newUsage);

} /* Rendering */

#endif /* end of include guard: RENDERING_CORE_INTERNAL_VKUTILS_H_ */
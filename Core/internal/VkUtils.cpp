/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "../Common.h"
#include "../../State/ShaderLayout.h"
#include <Util/Resources/ResourceFormat.h>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace Rendering {

//-----------------

vk::Format getVkFormat(const InternalFormat& format) {
	switch(format) {
		case InternalFormat::R8Unorm: return vk::Format::eR8Unorm;
		case InternalFormat::R8Snorm: return vk::Format::eR8Snorm;
		case InternalFormat::R16Unorm: return vk::Format::eR16Unorm;
		case InternalFormat::R16Snorm: return vk::Format::eR16Snorm;
		case InternalFormat::RG8Unorm: return vk::Format::eR8G8Unorm;
		case InternalFormat::RG8Snorm: return vk::Format::eR8G8Snorm;
		case InternalFormat::RG16Unorm: return vk::Format::eR16G16Unorm;
		case InternalFormat::RG16Snorm: return vk::Format::eR16G16Snorm;
		case InternalFormat::RGB16Unorm: return vk::Format::eR16G16B16Unorm;
		case InternalFormat::RGB16Snorm: return vk::Format::eR16G16B16Snorm;
		case InternalFormat::RGB5A1Unorm: return vk::Format::eA1R5G5B5UnormPack16;
		case InternalFormat::RGBA8Unorm: return vk::Format::eR8G8B8A8Unorm;
		case InternalFormat::RGBA8Snorm: return vk::Format::eR8G8B8A8Snorm;
		case InternalFormat::RGB10A2Unorm: return vk::Format::eA2R10G10B10UnormPack32;
		case InternalFormat::RGB10A2Uint: return vk::Format::eA2R10G10B10UintPack32;
		case InternalFormat::RGBA16Unorm: return vk::Format::eR16G16B16A16Unorm;
		case InternalFormat::RGBA8UnormSrgb: return vk::Format::eR8G8B8A8Srgb;
		case InternalFormat::R16Float: return vk::Format::eR16Sfloat;
		case InternalFormat::RG16Float: return vk::Format::eR16G16Sfloat;
		case InternalFormat::RGB16Float: return vk::Format::eR16G16B16Sfloat;
		case InternalFormat::RGBA16Float: return vk::Format::eR16G16B16A16Sfloat;
		case InternalFormat::R32Float: return vk::Format::eR32Sfloat;
		case InternalFormat::RG32Float: return vk::Format::eR32G32Sfloat;
		case InternalFormat::RGB32Float: return vk::Format::eR32G32B32Sfloat;
		case InternalFormat::RGBA32Float: return vk::Format::eR32G32B32A32Sfloat;
		case InternalFormat::R11G11B10Float: return vk::Format::eB10G11R11UfloatPack32;
		case InternalFormat::RGB9E5Float: return vk::Format::eE5B9G9R9UfloatPack32;
		case InternalFormat::R8Int: return vk::Format::eR8Sint;
		case InternalFormat::R8Uint: return vk::Format::eR8Uint;
		case InternalFormat::R16Int: return vk::Format::eR16Sint;
		case InternalFormat::R16Uint: return vk::Format::eR16Uint;
		case InternalFormat::R32Int: return vk::Format::eR32Sint;
		case InternalFormat::R32Uint: return vk::Format::eR32Uint;
		case InternalFormat::RG8Int: return vk::Format::eR8G8Sint;
		case InternalFormat::RG8Uint: return vk::Format::eR8G8Uint;
		case InternalFormat::RG16Int: return vk::Format::eR16G16Sint;
		case InternalFormat::RG16Uint: return vk::Format::eR16G16Uint;
		case InternalFormat::RG32Int: return vk::Format::eR32G32Sint;
		case InternalFormat::RG32Uint: return vk::Format::eR32G32Uint;
		case InternalFormat::RGB16Int: return vk::Format::eR16G16B16Sint;
		case InternalFormat::RGB16Uint: return vk::Format::eR16G16B16Uint;
		case InternalFormat::RGB32Int: return vk::Format::eR32G32B32Sint;
		case InternalFormat::RGB32Uint: return vk::Format::eR32G32B32Uint;
		case InternalFormat::RGBA8Int: return vk::Format::eR8G8B8A8Sint;
		case InternalFormat::RGBA8Uint: return vk::Format::eR8G8B8A8Uint;
		case InternalFormat::RGBA16Int: return vk::Format::eR16G16B16A16Sint;
		case InternalFormat::RGBA16Uint: return vk::Format::eR16G16B16A16Uint;
		case InternalFormat::RGBA32Int: return vk::Format::eR32G32B32A32Sint;
		case InternalFormat::RGBA32Uint: return vk::Format::eR32G32B32A32Uint;
		case InternalFormat::BGRA8Unorm: return vk::Format::eB8G8R8A8Unorm;
		case InternalFormat::BGRA8UnormSrgb: return vk::Format::eB8G8R8A8Srgb;
		case InternalFormat::R5G6B5Unorm: return vk::Format::eR5G6B5UnormPack16;
		case InternalFormat::D32Float: return vk::Format::eD32Sfloat;
		case InternalFormat::D16Unorm: return vk::Format::eD16Unorm;
		case InternalFormat::D32FloatS8X24: return vk::Format::eD32SfloatS8Uint;
		case InternalFormat::D24UnormS8: return vk::Format::eD24UnormS8Uint;
		case InternalFormat::BC1Unorm: return vk::Format::eBc1RgbUnormBlock;
		case InternalFormat::BC1UnormSrgb: return vk::Format::eBc1RgbSrgbBlock;
		case InternalFormat::BC2Unorm: return vk::Format::eBc2UnormBlock;
		case InternalFormat::BC2UnormSrgb: return vk::Format::eBc2SrgbBlock;
		case InternalFormat::BC3Unorm: return vk::Format::eBc3UnormBlock;
		case InternalFormat::BC3UnormSrgb: return vk::Format::eBc3SrgbBlock;
		case InternalFormat::BC4Unorm: return vk::Format::eBc4UnormBlock;
		case InternalFormat::BC4Snorm: return vk::Format::eBc4SnormBlock;
		case InternalFormat::BC5Unorm: return vk::Format::eBc5UnormBlock;
		case InternalFormat::BC5Snorm: return vk::Format::eBc5SnormBlock;
		case InternalFormat::BC6HS16: return vk::Format::eBc6HSfloatBlock;
		case InternalFormat::BC6HU16: return vk::Format::eBc6HUfloatBlock;
		case InternalFormat::BC7Unorm: return vk::Format::eBc7UnormBlock;
		case InternalFormat::BC7UnormSrgb: return vk::Format::eBc7SrgbBlock;
		default: return vk::Format::eUndefined;
	}
}

//-----------------

vk::AccessFlags getVkAccessMask(const ResourceUsage& usage) {
	switch(usage) {
		case ResourceUsage::Undefined:
		case ResourceUsage::PreInitialized:
		case ResourceUsage::Present:
		case ResourceUsage::General: return {};
		case ResourceUsage::RenderTarget: return vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
		case ResourceUsage::DepthStencil: return vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
		case ResourceUsage::ShaderResource: return vk::AccessFlagBits::eInputAttachmentRead;
		case ResourceUsage::CopySource: return vk::AccessFlagBits::eTransferRead;
		case ResourceUsage::CopyDestination: return vk::AccessFlagBits::eTransferWrite;
		case ResourceUsage::ShaderWrite: return vk::AccessFlagBits::eShaderWrite;
		default: return {};
	};
}
//-----------------

vk::Filter getVkFilter(const ImageFilter& filter) {
	switch(filter) {
		case Nearest: return vk::Filter::eNearest;
		case Linear: return vk::Filter::eLinear;
		default: return {};
	}
};

//-----------------

vk::SamplerMipmapMode getVkMipmapMode(const ImageFilter& filter) {
	switch(filter) {
		case Nearest: return vk::SamplerMipmapMode::eNearest;
		case Linear: return vk::SamplerMipmapMode::eLinear;
		default: return {};
	}
};

//-----------------

vk::SamplerAddressMode getVkAddressMode(const ImageAddressMode& filter) {
	switch(filter) {
		case Repeat: return vk::SamplerAddressMode::eRepeat;
		case MirroredRepeat: return vk::SamplerAddressMode::eMirroredRepeat;
		case ClampToEdge: return vk::SamplerAddressMode::eClampToEdge;
		case ClampToBorder: return vk::SamplerAddressMode::eClampToBorder;
		default: return {};
	}
};

//-----------------

vk::CompareOp getVkCompareOp(const ComparisonFunc& op) {
	switch(op) {
		case ComparisonFunc::Never: return vk::CompareOp::eNever;
		case ComparisonFunc::Less: return vk::CompareOp::eLess;
		case ComparisonFunc::Equal: return vk::CompareOp::eEqual;
		case ComparisonFunc::LessOrEqual: return vk::CompareOp::eLessOrEqual;
		case ComparisonFunc::Greater: return vk::CompareOp::eGreater;
		case ComparisonFunc::NotEqual: return vk::CompareOp::eNotEqual;
		case ComparisonFunc::GreaterOrEqual: return vk::CompareOp::eGreaterOrEqual;
		case ComparisonFunc::Always: return vk::CompareOp::eAlways;
		default: return vk::CompareOp::eNever;
	}
}

//-----------------

vk::ImageLayout getVkImageLayout(const ResourceUsage& usage) {
	switch(usage) {
		case ResourceUsage::Undefined: return vk::ImageLayout::eUndefined;
		case ResourceUsage::PreInitialized: return vk::ImageLayout::ePreinitialized;
		case ResourceUsage::ShaderWrite:
		case ResourceUsage::General: return vk::ImageLayout::eGeneral;
		case ResourceUsage::RenderTarget: return vk::ImageLayout::eColorAttachmentOptimal;
		case ResourceUsage::DepthStencil: return vk::ImageLayout::eDepthStencilAttachmentOptimal;
		case ResourceUsage::ShaderResource: return vk::ImageLayout::eShaderReadOnlyOptimal;
		case ResourceUsage::CopySource: return vk::ImageLayout::eTransferSrcOptimal;
		case ResourceUsage::CopyDestination: return vk::ImageLayout::eTransferDstOptimal;
		case ResourceUsage::Present: return vk::ImageLayout::ePresentSrcKHR;
		default: return vk::ImageLayout::eUndefined;
	};
}

//-----------------

vk::PipelineStageFlags getVkPipelineStageMask(const ResourceUsage& usage, bool src) {
	switch(usage) {
		case ResourceUsage::Undefined:
		case ResourceUsage::PreInitialized:
		case ResourceUsage::General: return src ? vk::PipelineStageFlagBits::eTopOfPipe : (vk::PipelineStageFlagBits::eAllGraphics | vk::PipelineStageFlagBits::eAllCommands);
		case ResourceUsage::RenderTarget: return vk::PipelineStageFlagBits::eColorAttachmentOutput;
		case ResourceUsage::DepthStencil: return src ? vk::PipelineStageFlagBits::eLateFragmentTests : vk::PipelineStageFlagBits::eEarlyFragmentTests;
		case ResourceUsage::ShaderWrite:
		case ResourceUsage::ShaderResource: return vk::PipelineStageFlagBits::eFragmentShader | vk::PipelineStageFlagBits::eComputeShader;
		case ResourceUsage::CopySource:
		case ResourceUsage::CopyDestination: return vk::PipelineStageFlagBits::eTransfer;
		case ResourceUsage::Present: return src ? (vk::PipelineStageFlagBits::eAllGraphics | vk::PipelineStageFlagBits::eAllCommands) : vk::PipelineStageFlagBits::eTopOfPipe;
		default: return vk::PipelineStageFlagBits::eAllGraphics | vk::PipelineStageFlagBits::eAllCommands;
	};
}

//-----------------

vk::BufferUsageFlags getVkBufferUsage(const ResourceUsage& usage) {
	vk::BufferUsageFlags flags = vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst;
	switch(usage) {
		case ResourceUsage::ShaderResource: flags |= vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eUniformTexelBuffer; break;
		case ResourceUsage::ShaderWrite: flags |= vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eStorageTexelBuffer; break;
		case ResourceUsage::IndexBuffer: flags |= vk::BufferUsageFlagBits::eIndexBuffer; break;
		case ResourceUsage::VertexBuffer: flags |= vk::BufferUsageFlagBits::eVertexBuffer; break;
		case ResourceUsage::IndirectBuffer: flags |= vk::BufferUsageFlagBits::eIndirectBuffer; break;
		case ResourceUsage::General: flags |= vk::BufferUsageFlagBits::eUniformBuffer |
			vk::BufferUsageFlagBits::eUniformTexelBuffer |
			vk::BufferUsageFlagBits::eStorageBuffer |
			vk::BufferUsageFlagBits::eStorageTexelBuffer |
			vk::BufferUsageFlagBits::eIndexBuffer |
			vk::BufferUsageFlagBits::eVertexBuffer |
			vk::BufferUsageFlagBits::eIndirectBuffer;
		default: break;
	}
	
	return flags;
}

//-----------------

vk::ImageUsageFlags getVkImageUsage(const ResourceUsage& usage) {
	vk::ImageUsageFlags flags = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;
	switch(usage) {
		case ResourceUsage::ShaderResource: flags |= vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eInputAttachment; break;
		case ResourceUsage::ShaderWrite: flags |= vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eInputAttachment; break;
		case ResourceUsage::Present:
		case ResourceUsage::RenderTarget: flags |= vk::ImageUsageFlagBits::eColorAttachment; break;
		case ResourceUsage::DepthStencil: flags |= vk::ImageUsageFlagBits::eDepthStencilAttachment; break;
		case ResourceUsage::General: flags |= vk::ImageUsageFlagBits::eColorAttachment |
			vk::ImageUsageFlagBits::eDepthStencilAttachment |
			vk::ImageUsageFlagBits::eInputAttachment |
			vk::ImageUsageFlagBits::eSampled |
			vk::ImageUsageFlagBits::eStorage |
			vk::ImageUsageFlagBits::eTransientAttachment;
		default: break;
	}
	
	return flags;
}

//-----------------

vk::DescriptorType getVkDescriptorType(const ShaderResourceType& type, bool dynamic) {
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

vk::ShaderStageFlags getVkStageFlags(const ShaderStage& stages) {
	vk::ShaderStageFlags flags;
	if((stages & ShaderStage::Vertex) == ShaderStage::Vertex) flags |= vk::ShaderStageFlagBits::eVertex;
	if((stages & ShaderStage::TessellationControl) == ShaderStage::TessellationControl) flags |= vk::ShaderStageFlagBits::eTessellationControl;
	if((stages & ShaderStage::TessellationEvaluation) == ShaderStage::TessellationEvaluation) flags |= vk::ShaderStageFlagBits::eTessellationEvaluation;
	if((stages & ShaderStage::Geometry) == ShaderStage::Geometry) flags |= vk::ShaderStageFlagBits::eGeometry;
	if((stages & ShaderStage::Fragment) == ShaderStage::Fragment) flags |= vk::ShaderStageFlagBits::eFragment;
	if((stages & ShaderStage::Compute) == ShaderStage::Compute) flags |= vk::ShaderStageFlagBits::eCompute;
	return flags;
}

} /* Rendering */
/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "CopyCommands.h"
#include "../Device.h"
#include "../BufferStorage.h"
#include "../ImageView.h"
#include "../ImageStorage.h"
#include "../../Buffer/BufferObject.h"
#include "../../Texture/Texture.h"
#include "../internal/VkUtils.h"

#include <Util/Macros.h>

namespace Rendering {

//--------------

CopyBufferCommand::CopyBufferCommand(const BufferStorageRef& srcBuffer, const BufferStorageRef& tgtBuffer, size_t size, size_t srcOffset, size_t tgtOffset) :
	srcBuffer(BufferObject::create(srcBuffer)), tgtBuffer(BufferObject::create(tgtBuffer)), size(size), srcOffset(srcOffset), tgtOffset(tgtOffset) {}

//--------------

CopyBufferCommand::~CopyBufferCommand() = default;

//--------------

bool CopyBufferCommand::compile(CompileContext& context) {
	WARN_AND_RETURN_IF(!srcBuffer || !tgtBuffer || !srcBuffer->isValid() || !tgtBuffer->isValid(), "Cannot copy buffer. Invalid buffers.", false);
	static_cast<vk::CommandBuffer>(context.cmd).copyBuffer({srcBuffer->getApiHandle()}, {tgtBuffer->getApiHandle()}, {{srcOffset,tgtOffset,size}});
	return true;
}

//--------------

UpdateBufferCommand::UpdateBufferCommand(const BufferStorageRef& buffer, const uint8_t* data, size_t size, size_t tgtOffset) :
	srcData(data, data+size), tgtBuffer(BufferObject::create(buffer)), tgtOffset(tgtOffset) {}

//--------------

UpdateBufferCommand::~UpdateBufferCommand() = default;

//--------------

bool UpdateBufferCommand::compile(CompileContext& context) {
	WARN_AND_RETURN_IF(!tgtBuffer || !tgtBuffer->isValid() || srcData.empty(), "Cannot update buffer. Invalid buffer or data.", false);
	size_t offset = tgtBuffer->getOffset() + tgtOffset;
	WARN_AND_RETURN_IF(srcData.size()+offset > tgtBuffer->getSize(), "Cannot update buffer. Offset+size exceeds buffer size.", false);
	static_cast<vk::CommandBuffer>(context.cmd).updateBuffer({tgtBuffer->getApiHandle()}, offset, srcData.size(), srcData.data());
	return true;
}

//--------------

CopyImageCommand::~CopyImageCommand() = default;

//--------------

bool CopyImageCommand::compile(CompileContext& context) {
	WARN_AND_RETURN_IF(!srcImage || !tgtImage, "Cannot copy image. Invalid images.", false);
	WARN_AND_RETURN_IF(srcRegion.extent != tgtRegion.extent, "Cannot copy image. Source and target extent must be the same.", false);
	transferImageLayout(context.cmd, srcImage, ResourceUsage::CopySource);
	transferImageLayout(context.cmd, tgtImage, ResourceUsage::CopyDestination);

	vk::ImageAspectFlags srcAspect = isDepthStencilFormat(srcImage->getFormat()) ? (vk::ImageAspectFlagBits::eDepth |  vk::ImageAspectFlagBits::eStencil) : vk::ImageAspectFlagBits::eColor;
	vk::ImageAspectFlags tgtAspect = isDepthStencilFormat(tgtImage->getFormat()) ? (vk::ImageAspectFlagBits::eDepth |  vk::ImageAspectFlagBits::eStencil) : vk::ImageAspectFlagBits::eColor;
	vk::ImageCopy copyRegion{
		{srcAspect, srcRegion.mipLevel, srcRegion.baseLayer, srcRegion.layerCount}, {srcRegion.offset.x(), srcRegion.offset.y(), srcRegion.offset.z()},
		{tgtAspect, tgtRegion.mipLevel, tgtRegion.baseLayer, tgtRegion.layerCount}, {tgtRegion.offset.x(), tgtRegion.offset.y(), tgtRegion.offset.z()},
		{srcRegion.extent.x(), srcRegion.extent.y(), srcRegion.extent.z()}
	};
	static_cast<vk::CommandBuffer>(context.cmd).copyImage(
		static_cast<vk::Image>(srcImage->getApiHandle()), getVkImageLayout(ResourceUsage::CopySource),
		static_cast<vk::Image>(tgtImage->getApiHandle()), getVkImageLayout(ResourceUsage::CopyDestination),
		{copyRegion}
	);
	return true;
}

//--------------

CopyBufferToImageCommand::~CopyBufferToImageCommand() = default;

//--------------

bool CopyBufferToImageCommand::compile(CompileContext& context) {
	WARN_AND_RETURN_IF(!srcBuffer || !tgtImage, "Cannot copy buffer to image. Invalid buffer or image.", false);
	transferImageLayout(context.cmd, tgtImage, ResourceUsage::CopyDestination);

	vk::ImageAspectFlags tgtAspect = isDepthStencilFormat(tgtImage->getFormat()) ? (vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil) : vk::ImageAspectFlagBits::eColor;
	vk::BufferImageCopy copyRegion{
		static_cast<vk::DeviceSize>(srcOffset), 0u, 0u,
		{tgtAspect, tgtRegion.mipLevel, tgtRegion.baseLayer, tgtRegion.layerCount}, {tgtRegion.offset.x(), tgtRegion.offset.y(), tgtRegion.offset.z()},
		{tgtRegion.extent.x(), tgtRegion.extent.y(), tgtRegion.extent.z()}
	};
	static_cast<vk::CommandBuffer>(context.cmd).copyBufferToImage(
		static_cast<vk::Buffer>(srcBuffer->getApiHandle()),
		static_cast<vk::Image>(tgtImage->getApiHandle()), getVkImageLayout(ResourceUsage::CopyDestination),
		{copyRegion}
	);
	return true;
}

//--------------

CopyImageToBufferCommand::~CopyImageToBufferCommand() = default;

//--------------

bool CopyImageToBufferCommand::compile(CompileContext& context) {
	WARN_AND_RETURN_IF(!srcImage || !tgtBuffer, "Cannot copy image to buffer. Invalid buffer or image.", false);
	transferImageLayout(context.cmd, srcImage, ResourceUsage::CopySource);

	vk::ImageAspectFlags srcAspect = isDepthStencilFormat(srcImage->getFormat()) ? (vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil) : vk::ImageAspectFlagBits::eColor;
	vk::BufferImageCopy copyRegion{
		static_cast<vk::DeviceSize>(tgtOffset), 0u, 0u,
		{srcAspect, srcRegion.mipLevel, srcRegion.baseLayer, srcRegion.layerCount}, {srcRegion.offset.x(), srcRegion.offset.y(), srcRegion.offset.z()},
		{srcRegion.extent.x(), srcRegion.extent.y(), srcRegion.extent.z()}
	};
	static_cast<vk::CommandBuffer>(context.cmd).copyImageToBuffer(
		static_cast<vk::Image>(srcImage->getApiHandle()), getVkImageLayout(ResourceUsage::CopySource),
		static_cast<vk::Buffer>(tgtBuffer->getApiHandle()),
		{copyRegion}
	);
	return true;
}

//--------------

BlitImageCommand::~BlitImageCommand() = default;

//--------------

bool BlitImageCommand::compile(CompileContext& context) {
	WARN_AND_RETURN_IF(!srcImage || !tgtImage, "Cannot blit image. Invalid images.", false);
	transferImageLayout(context.cmd, srcImage, ResourceUsage::CopySource);
	transferImageLayout(context.cmd, tgtImage, ResourceUsage::CopyDestination);

	vk::ImageAspectFlags srcAspect = isDepthStencilFormat(srcImage->getFormat()) ? (vk::ImageAspectFlagBits::eDepth |  vk::ImageAspectFlagBits::eStencil) : vk::ImageAspectFlagBits::eColor;
	vk::ImageAspectFlags tgtAspect = isDepthStencilFormat(tgtImage->getFormat()) ? (vk::ImageAspectFlagBits::eDepth |  vk::ImageAspectFlagBits::eStencil) : vk::ImageAspectFlagBits::eColor;
	Geometry::Vec3i srcOffset2 = srcRegion.offset + toVec3i(srcRegion.extent);
	Geometry::Vec3i tgtOffset2 = srcRegion.offset + toVec3i(srcRegion.extent);
	vk::ImageBlit blitRegion{
		{srcAspect, srcRegion.mipLevel, srcRegion.baseLayer, srcRegion.layerCount},
		{vk::Offset3D{srcRegion.offset.x(), srcRegion.offset.y(), srcRegion.offset.z()}, vk::Offset3D{srcOffset2.x(), srcOffset2.y(), srcOffset2.z()}},
		{tgtAspect, tgtRegion.mipLevel, tgtRegion.baseLayer, tgtRegion.layerCount},
		{vk::Offset3D{tgtRegion.offset.x(), tgtRegion.offset.y(), tgtRegion.offset.z()}, vk::Offset3D{tgtOffset2.x(), tgtOffset2.y(), tgtOffset2.z()}},
	};
	static_cast<vk::CommandBuffer>(context.cmd).blitImage(
		static_cast<vk::Image>(srcImage->getApiHandle()), getVkImageLayout(ResourceUsage::CopySource),
		static_cast<vk::Image>(tgtImage->getApiHandle()), getVkImageLayout(ResourceUsage::CopyDestination),
		{blitRegion}, getVkFilter(filter)
	);
	return true;
}

//--------------

ClearImageCommand::ClearImageCommand(const TextureRef& texture, const Util::Color4f& color) : view(texture->getImageView()), image(nullptr), color(color) {}

//--------------

ClearImageCommand::~ClearImageCommand() = default;

//--------------

bool ClearImageCommand::compile(CompileContext& context) {
	WARN_AND_RETURN_IF(!image && !view, "Cannot clear image. Invalid image or image view.", false);
	ImageStorageRef img = image ? image : view->getImage();
	transferImageLayout(context.cmd, img, ResourceUsage::CopyDestination);

	const auto& format = image->getFormat();
	auto layout = getVkImageLayout(ResourceUsage::CopyDestination);
	vk::ImageSubresourceRange range{};
	if(view) {
		range.baseMipLevel = view->getMipLevel();
		range.levelCount = view->getMipLevelCount();
		range.baseArrayLayer = view->getLayer();
		range.layerCount = view->getLayerCount();
	} else {
		range.levelCount = format.mipLevels;
		range.layerCount = format.layers;
	}

	vk::CommandBuffer vkCmd(context.cmd);
	if(isDepthStencilFormat(img->getFormat())) {
		vk::ClearDepthStencilValue clearValue{};
		clearValue.depth = color.r();
		clearValue.stencil = static_cast<uint32_t>(color.g());
		range.aspectMask = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
		vkCmd.clearDepthStencilImage(static_cast<vk::Image>(img->getApiHandle()), layout, clearValue, {range});
	} else {
		vk::ClearColorValue clearValue{};
		clearValue.setFloat32({color.r(), color.g(), color.b(), color.a()});
		range.aspectMask = vk::ImageAspectFlagBits::eColor;
		vkCmd.clearColorImage(static_cast<vk::Image>(img->getApiHandle()), layout, clearValue, {range});
	};
	return true;
}

//--------------

} /* Rendering */
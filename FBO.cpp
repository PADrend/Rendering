/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	Copyright (C) 2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "FBO.h"
#include "Core/Device.h"
#include "Core/ImageStorage.h"
#include "Core/ImageView.h"
#include "Core/Sampler.h"
#include "Texture/Texture.h"
#include "GLHeader.h"

#include <Util/Macros.h>
#include <Util/Utils.h>

#include <stdexcept>

namespace Rendering {

//-----------------

FBO::Ref FBO::create(uint32_t maxAttachments) { return new FBO(maxAttachments); }

//-----------------

FBO::FBO(uint32_t maxAttachments) : colorAttachments(maxAttachments, nullptr) {}

//-----------------

FBO::~FBO() = default;

//-----------------

void FBO::attachColorTexture(const TextureRef& texture, uint32_t index) {
	WARN_AND_RETURN_IF(index >= colorAttachments.size(),"FBO: invalid attachment index " + std::to_string(index) + ". Maximum number of attachments is " + std::to_string(colorAttachments.size()) + ".",);
	if(!texture) {
		detachColorTexture(index);
		return;
	}
	colorAttachments[index] = texture;
	width = texture->getWidth();
	height = texture->getHeight();
}

//-----------------

void FBO::attachColorTexture(const ImageViewRef& view, uint32_t index) {
	WARN_AND_RETURN_IF(index >= colorAttachments.size(),"FBO: invalid attachment index " + std::to_string(index) + ". Maximum number of attachments is " + std::to_string(colorAttachments.size()) + ".",);
	if(!view) {
		detachColorTexture(index);
		return;
	}
	attachColorTexture(Texture::create(view->getImage()->getDevice(), view), index);
}

//-----------------

void FBO::attachColorTexture(const ImageStorageRef& image, uint32_t index, uint32_t mipLevel, uint32_t baseLayer, uint32_t layerCount) {
	WARN_AND_RETURN_IF(index >= colorAttachments.size(),"FBO: invalid attachment index " + std::to_string(index) + ". Maximum number of attachments is " + std::to_string(colorAttachments.size()) + ".",);
	if(!image) {
		detachColorTexture(index);
		return;
	}
	attachColorTexture(ImageView::create(image, {image->getType(), mipLevel, 1u, baseLayer, layerCount}), index);
}

//-----------------

void FBO::detachColorTexture(uint32_t index) {
	if(index < colorAttachments.size())
		colorAttachments[index] = nullptr;
}

//-----------------


void FBO::attachDepthStencilTexture(const TextureRef& texture) {
	depthStencilAttachment = texture;
	if(texture) {
		width = texture->getWidth();
		height = texture->getHeight();
	}
}

//-----------------

void FBO::attachDepthStencilTexture(const ImageViewRef& view) {
	if(!view) {
		detachDepthStencilTexture();
		return;
	}
	attachDepthStencilTexture(Texture::create(view->getImage()->getDevice(), view));
}

//-----------------

void FBO::attachDepthStencilTexture(const ImageStorageRef& image, uint32_t mipLevel, uint32_t baseLayer, uint32_t layerCount) {
	if(!image) {
		detachDepthStencilTexture();
		return;
	}
	attachDepthStencilTexture(ImageView::create(image, {image->getType(), mipLevel, 1u, baseLayer, layerCount}));
}

//-----------------

void FBO::detachDepthStencilTexture() {
	depthStencilAttachment = nullptr;
}

//-----------------

 TextureRef FBO::getColorAttachment(uint32_t index) const {
	return index < colorAttachments.size() ? colorAttachments[index] : nullptr;
}

//-----------------

TextureRef FBO::getDepthStencilAttachment() const {
	return depthStencilAttachment;
}

//-----------------

bool FBO::isValid() const {
	if(width == 0 || height == 0)
		return false;
	for(uint32_t i=0; i<colorAttachments.size(); ++i) {
		const auto& att = colorAttachments[i];
		if(att && (att->getWidth() != width || att->getHeight() != height)) {
			WARN("FBO: Invalid size of color attachment " + std::to_string(i) + ".");
			return false;
		}
	}
	if(depthStencilAttachment && (depthStencilAttachment->getWidth() != width || depthStencilAttachment->getHeight() != height)) {
		WARN("FBO: Invalid size of depth attachment.");
		return false;
	}
	return true;
}

//=========================================================================
// deprecated

void FBO::attachTexture(RenderingContext & context,uint32_t attachmentPoint,Texture * texture,uint32_t level,int32_t layer) {
	if(attachmentPoint == GL_DEPTH_ATTACHMENT || attachmentPoint == GL_STENCIL_ATTACHMENT) {
		attachDepthStencilTexture(context, texture, level, layer);
	} else if(attachmentPoint >= GL_COLOR_ATTACHMENT0) {
		attachColorTexture(context, texture, attachmentPoint, level, layer);
	}
}
void FBO::detachTexture(RenderingContext & context, uint32_t attachmentPoint) { 
	attachTexture(context,attachmentPoint,nullptr,0,-1); 
}
void FBO::attachColorTexture(RenderingContext & context, Texture * texture, uint32_t colorBufferId,uint32_t level,int32_t layer) {
	if(!texture) {
		detachColorTexture(colorBufferId);
		return;
	} else if(!texture->isValid())
		texture->upload();

	uint32_t baseLayer = layer < 0 ? 0 : layer;
	uint32_t layerCount = layer < 0 ? 0 : 1;
	auto& view = texture->getImageView();
	if(view->getLayer() == baseLayer && view->getLayerCount() == layerCount && view->getMipLevel() == level) {
		attachColorTexture(texture);
	} else {
		attachColorTexture(view);
	}
}
void FBO::detachColorTexture(RenderingContext & context, uint32_t colorBufferId) {
	detachColorTexture(colorBufferId);
}
void FBO::attachDepthStencilTexture(RenderingContext & context, Texture * texture,uint32_t level,int32_t layer) {
	if(!texture) {
		detachDepthStencilTexture();
		return;
	} else if(!texture->isValid())
		texture->upload();

	uint32_t baseLayer = layer < 0 ? 0 : layer;
	uint32_t layerCount = layer < 0 ? 0 : 1;
	auto& view = texture->getImageView();
	if(view->getLayer() == baseLayer && view->getLayerCount() == layerCount && view->getMipLevel() == level) {
		attachDepthStencilTexture(texture);
	} else {
		attachDepthStencilTexture(view);
	}
}

void FBO::detachDepthStencilTexture(RenderingContext & context) {
	detachDepthStencilTexture();
}
void FBO::attachDepthTexture(RenderingContext & context, Texture * t,uint32_t level,int32_t layer) {
	attachDepthStencilTexture(context, t,level,layer);
}
void FBO::detachDepthTexture(RenderingContext & context) {
	detachDepthStencilTexture();
}

void FBO::setDrawBuffers(RenderingContext & context, uint32_t number) {
}

void FBO::blitToScreen(RenderingContext & context, const Geometry::Rect_i& srcRect, const Geometry::Rect_i& tgtRect) {
}

}

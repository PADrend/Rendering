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
#include "RenderingContext/RenderingContext.h"
#include "Texture/Texture.h"
#include "GLHeader.h"
#include "Helper.h"
#include <stdexcept>

namespace Rendering {


FBO::FBO() : glId(0){
	//ctor
}

FBO::~FBO() {
	if(glId!=0){
		glDeleteFramebuffers(1, &glId);
		glId=0;
	}
	//dtor
}

void FBO::prepare(){
	if(glId==0) {
		glCreateFramebuffers(1, &glId);
	}
}

void FBO::bind(){
	prepare();
	glBindFramebuffer(GL_FRAMEBUFFER,glId);
}

void FBO::unbind(){
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FBO::attachTexture(RenderingContext & context, GLenum attachmentPoint, Texture * texture, uint32_t level, int32_t layer) {
	prepare();
	if( texture ){
		GLuint textureId = texture->_prepareForBinding(context);
		if(layer+1 > texture->getNumLayers())
			throw std::invalid_argument("FBO::attachTexture: invalid texture layer.");
		
		if(layer >= 0)
			glNamedFramebufferTextureLayer(glId, attachmentPoint, textureId, level, layer);
		else
			glNamedFramebufferTexture(glId, attachmentPoint, textureId, level);
		/*bind();
		switch( texture->getTextureType() ){
			case TextureType::TEXTURE_1D:
				glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentPoint, texture->getGLTextureType(),  textureId, level);	// GL_framebuffer_object
				break;
			case TextureType::TEXTURE_2D:
			case TextureType::TEXTURE_2D_MULTISAMPLE:
				glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentPoint, texture->getGLTextureType(),  textureId, level);	// GL_framebuffer_object
				break;
			case TextureType::TEXTURE_CUBE_MAP:
				glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentPoint, GL_TEXTURE_CUBE_MAP_POSITIVE_X+std::max(0,layer), 	textureId, level);	// GL_framebuffer_object
				break;
			case TextureType::TEXTURE_1D_ARRAY:
			case TextureType::TEXTURE_2D_ARRAY:
			case TextureType::TEXTURE_3D:
			case TextureType::TEXTURE_CUBE_MAP_ARRAY:{
				if(layer >= 0)
					glFramebufferTextureLayer(GL_FRAMEBUFFER, attachmentPoint, textureId, level, layer);				// GL_ARB_framebuffer_object
				else 
					glFramebufferTexture(GL_FRAMEBUFFER, attachmentPoint, textureId, level);	
				break;
			}
			case TextureType::TEXTURE_BUFFER:
				throw std::logic_error("FBO::attachTexture: TextureBuffers are no valid targets.");
			default:
				throw std::logic_error("FBO::attachTexture: ???");
		}
		unbind();*/
	}else{
		glNamedFramebufferTexture(glId, attachmentPoint, 0, 0);
		//glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentPoint,GL_TEXTURE_2D,0,0);
	}
	GET_GL_ERROR();
}

void FBO::attachColorTexture(RenderingContext & context, Texture * t, uint32_t colorBufferId,uint32_t level,int32_t layer) {
	attachTexture(context, GL_COLOR_ATTACHMENT0 + colorBufferId, t,level,layer);
}
void FBO::detachColorTexture(RenderingContext & context, uint32_t colorBufferId) {
	detachTexture(context, GL_COLOR_ATTACHMENT0 + colorBufferId);
}
void FBO::attachDepthStencilTexture(RenderingContext & context, Texture * t,uint32_t level,int32_t layer) {
	attachTexture(context, GL_DEPTH_ATTACHMENT, t,level,layer);
	attachTexture(context, GL_STENCIL_ATTACHMENT, t,level,layer);
}
void FBO::detachDepthStencilTexture(RenderingContext & context) {
	detachTexture(context, GL_DEPTH_ATTACHMENT);
	detachTexture(context, GL_STENCIL_ATTACHMENT);
}
void FBO::attachDepthTexture(RenderingContext & context, Texture * t,uint32_t level,int32_t layer) {
	attachTexture(context, GL_DEPTH_ATTACHMENT, t,level,layer);
}
void FBO::detachDepthTexture(RenderingContext & context) {
	detachTexture(context, GL_DEPTH_ATTACHMENT);
}

bool FBO::isComplete() {
	prepare();
	bool b = glCheckNamedFramebufferStatus(glId, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
	GET_GL_ERROR();
	return b;
}

const char * FBO::getStatusMessage() {
	const GLenum result = glCheckNamedFramebufferStatus(glId, GL_FRAMEBUFFER);
	GET_GL_ERROR();
	switch(result) {
		case GL_FRAMEBUFFER_COMPLETE:
			return "Framebuffer complete.";
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			return "[ERROR] Framebuffer incomplete: Attachment is NOT complete.";
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			return "[ERROR] Framebuffer incomplete: No image is attached to FBO.";
		case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
			return "[ERROR] Framebuffer incomplete: Attached images have different dimensions.";
		case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
			return "[ERROR] Framebuffer incomplete: Color attached images have different internal formats.";
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
			return "[ERROR] Framebuffer incomplete: Draw buffer.";
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
			return "[ERROR] Framebuffer incomplete: Read buffer.";
		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
			return "[ERROR] Framebuffer incomplete: Sample count or sample locations are not the same for all renderbuffers.";
		case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
			return "[ERROR] Framebuffer incomplete: Mixing of layered and unlayered attachments is not allowed.";
		case GL_FRAMEBUFFER_UNDEFINED:
			return "[ERROR] Default framebuffer does not exist.";
		case GL_FRAMEBUFFER_UNSUPPORTED:
			return "[ERROR] Unsupported by FBO implementation.";
		default:
			break;
	}
	return "[ERROR] Unknown error.";
}

void FBO::setDrawBuffers(uint32_t number) {
	static const GLenum buffers[] = {
		GL_COLOR_ATTACHMENT0,
		GL_COLOR_ATTACHMENT1,
		GL_COLOR_ATTACHMENT2,
		GL_COLOR_ATTACHMENT3,
		GL_COLOR_ATTACHMENT4,
		GL_COLOR_ATTACHMENT5,
		GL_COLOR_ATTACHMENT6,
		GL_COLOR_ATTACHMENT7
	};
	if(number > 8) {
		throw std::invalid_argument("Unsupported number of draw buffers requested");
	}
	glNamedFramebufferDrawBuffers(glId, static_cast<GLsizei>(number), buffers);
	GET_GL_ERROR();
}

void FBO::blit(RenderingContext & context, FBO* other, const Geometry::Rect_i& srcRect, const Geometry::Rect_i& tgtRect, bool includeDepth) {
	context.applyChanges();
	uint32_t flags = GL_COLOR_BUFFER_BIT | (includeDepth ? GL_DEPTH_BUFFER_BIT : 0);
	if(!other) glNamedFramebufferDrawBuffer(0, GL_BACK);
	glBlitNamedFramebuffer(glId, other ? other->getHandle() : 0, srcRect.getX(), srcRect.getY(), srcRect.getWidth(), srcRect.getHeight(), 
											tgtRect.getX(), tgtRect.getY(), tgtRect.getWidth(), tgtRect.getHeight(), 
											flags, GL_NEAREST);
	GET_GL_ERROR();
}

}

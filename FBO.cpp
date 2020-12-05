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
#include <iostream>

namespace Rendering {

//! (static)
void FBO::_disable(){
#if defined(LIB_GL)
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
#elif defined(LIB_GLESv2)
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif

}

FBO::FBO() : glId(0){
	//ctor
}

FBO::~FBO(){
	if(glId!=0){
#if defined(LIB_GL)
		glDeleteFramebuffers(1,&glId);
#elif defined(LIB_GLESv2)
		glDeleteFramebuffers(1,&glId);
#endif
		glId=0;
	}
	//dtor
}

void FBO::_enable(){
	if(glId==0){
#if defined(LIB_GL)
		glGenFramebuffers(1, &glId);
#elif defined(LIB_GLESv2)
		glGenFramebuffers(1, &glId);
#endif
	}
#if defined(LIB_GL)
	glBindFramebuffer(GL_FRAMEBUFFER,glId);
#elif defined(LIB_GLESv2)
	glBindFramebuffer(GL_FRAMEBUFFER,glId);
#endif
}

void FBO::attachTexture(RenderingContext & context,GLenum attachmentPoint,Texture * texture,uint32_t level,int32_t layer){
	context.pushAndSetFBO(this);
	if( texture ){
		GLuint textureId = texture->getGLId();
		if(textureId==0){
			texture->_uploadGLTexture(context);
			textureId = texture->getGLId();
		}
		if(layer+1 > texture->getNumLayers())
			throw std::invalid_argument("FBO::attachTexture: invalid texture layer.");
#if defined(LIB_GL)

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
				static const bool featureAvailable = isExtensionSupported("GL_ARB_framebuffer_object");	
				if(!featureAvailable)
					throw std::invalid_argument("FBO::attachTexture: texture type is not supported by your OpenGL version.");				
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
#elif defined(LIB_GLESv2)
		glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentPoint, texture->getGLTextureType(), textureId, level);
#endif
	}else{
#if defined(LIB_GL)
		glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentPoint,GL_TEXTURE_2D,0,0);
#elif defined(LIB_GLESv2)
		glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentPoint, GL_TEXTURE_2D, 0, 0);
#endif
	}
	context.popFBO();
	GET_GL_ERROR();
}


#if defined(LIB_GL)
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
#elif defined(LIB_GLESv2)
void FBO::attachColorTexture(RenderingContext & context, Texture * t, uint32_t colorBufferId, uint32_t level, int32_t layer) {
	if(colorBufferId != 0) {
		throw std::invalid_argument("Only one color attachment is supported in OpenGL ES 2.0.");
	}
	attachTexture(context, GL_COLOR_ATTACHMENT0, t, level, layer);
}
void FBO::detachColorTexture(RenderingContext & context, uint32_t colorBufferId) {
	if(colorBufferId != 0) {
		throw std::invalid_argument("Only one color attachment is supported in OpenGL ES 2.0.");
	}
	detachTexture(context, GL_COLOR_ATTACHMENT0);
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
#endif

bool FBO::isComplete(RenderingContext & context){
	context.pushAndSetFBO(this);
	bool b = false;
#if defined(LIB_GL)
	b = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
#elif defined(LIB_GLESv2)
	b = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
#endif
	context.popFBO();
	return b;
}

const char * FBO::getStatusMessage(RenderingContext & context) {
	context.pushAndSetFBO(this);
#if defined(LIB_GL)
	const GLenum result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
#elif defined(LIB_GLESv2)
	const GLenum result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
#endif
	context.popFBO();
#if defined(LIB_GL)
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
		case GL_FRAMEBUFFER_UNSUPPORTED:
			return "[ERROR] Unsupported by FBO implementation.";
		default:
			break;
	}
#elif defined(LIB_GLESv2)
	switch(result) {
		case GL_FRAMEBUFFER_COMPLETE:
			return "Framebuffer complete.";
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			return "[ERROR] Framebuffer incomplete: Attachment is NOT complete.";
		case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
			return "[ERROR] Framebuffer incomplete: Attached images have different dimensions.";
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			return "[ERROR] Framebuffer incomplete: No image is attached to FBO.";
		case GL_FRAMEBUFFER_UNSUPPORTED:
			return "[ERROR] Unsupported by FBO implementation.";
		default:
			break;
	}
#endif
	return "[ERROR] Unknown error.";
}

void FBO::setDrawBuffers(RenderingContext & context, uint32_t number) {
#ifdef LIB_GL
	context.pushAndSetFBO(this);
	context.applyChanges();
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
	glDrawBuffers(static_cast<GLsizei>(number), buffers);
	context.popFBO();
#endif /* LIB_GL */
}

void FBO::blitToScreen(RenderingContext & context, const Geometry::Rect_i& srcRect, const Geometry::Rect_i& tgtRect) {
#ifdef LIB_GL
	//context.pushFBO();
	//context.applyChanges();
	//glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	//glBindFramebuffer(GL_READ_FRAMEBUFFER, glId);
	//glDrawBuffer(GL_BACK);
	uint32_t writeBuffer = context.getActiveFBO() ? context.getActiveFBO()->getHandle() : 0;
	
	glBlitNamedFramebuffer(glId, writeBuffer, srcRect.getX(), srcRect.getY(), srcRect.getWidth(), srcRect.getHeight(), 
											tgtRect.getX(), tgtRect.getY(), tgtRect.getWidth(), tgtRect.getHeight(), 
											GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	GET_GL_ERROR();
	//context.popFBO();
#endif /* LIB_GL */
}

}

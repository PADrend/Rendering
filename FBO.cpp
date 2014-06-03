/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
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
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
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
		glDeleteFramebuffersEXT(1,&glId);
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
		glGenFramebuffersEXT(1, &glId);
#elif defined(LIB_GLESv2)
		glGenFramebuffers(1, &glId);
#endif
	}
#if defined(LIB_GL)
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,glId);
#elif defined(LIB_GLESv2)
	glBindFramebuffer(GL_FRAMEBUFFER,glId);
#endif
}

void FBO::attachTexture(RenderingContext & context,GLenum attachmentPoint,Texture * texture,uint32_t level,uint32_t layer){
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
		static const bool textureArrayAvailable = isExtensionSupported("GL_EXT_texture_array");
		if(textureArrayAvailable){
			if( texture->getNumLayers()>1 )
				glFramebufferTextureLayerEXT(GL_FRAMEBUFFER_EXT, attachmentPoint,  textureId, level, layer);
			else
				glFramebufferTextureEXT(GL_FRAMEBUFFER_EXT, attachmentPoint,  textureId, level);
		}else if( texture->getGLTextureType()==GL_TEXTURE_2D ){
				glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, attachmentPoint, texture->getGLTextureType(),  textureId, level);	
		}else{
			throw std::invalid_argument("FBO::attachTexture: texture type is not supported by your OpenGL version.");
		}
#elif defined(LIB_GLESv2)
		glFramebufferTexture2D(GL_FRAMEBUFFER, texture->getGLTextureType(),  textureId,0);
#endif
	}else{
#if defined(LIB_GL)
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, attachmentPoint,GL_TEXTURE_2D,0,0);
#elif defined(LIB_GLESv2)
		glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentPoint,GL_TEXTURE_2D,0,0);
#endif
	}
	context.popFBO();
	GET_GL_ERROR();
}


#if defined(LIB_GL)
void FBO::attachColorTexture(RenderingContext & context, Texture * t, uint32_t colorBufferId,uint32_t level,uint32_t layer) {
	attachTexture(context, GL_COLOR_ATTACHMENT0_EXT + colorBufferId, t,level,layer);
}
void FBO::detachColorTexture(RenderingContext & context, uint32_t colorBufferId) {
	detachTexture(context, GL_COLOR_ATTACHMENT0_EXT + colorBufferId);
}
void FBO::attachDepthStencilTexture(RenderingContext & context, Texture * t,uint32_t level,uint32_t layer) {
	attachTexture(context, GL_DEPTH_ATTACHMENT_EXT, t,level,layer);
	attachTexture(context, GL_STENCIL_ATTACHMENT_EXT, t,level,layer);
}
void FBO::detachDepthStencilTexture(RenderingContext & context) {
	detachTexture(context, GL_DEPTH_ATTACHMENT_EXT);
	detachTexture(context, GL_STENCIL_ATTACHMENT_EXT);
}
void FBO::attachDepthTexture(RenderingContext & context, Texture * t,uint32_t level,uint32_t layer) {
	attachTexture(context, GL_DEPTH_ATTACHMENT_EXT, t,level,layer);
}
void FBO::detachDepthTexture(RenderingContext & context) {
	detachTexture(context, GL_DEPTH_ATTACHMENT_EXT);
}
#elif defined(LIB_GLESv2)
void FBO::attachColorTexture(RenderingContext & context, Texture * t, uint32_t colorBufferId,uint32_t layer) {
	if(colorBufferId != 0) {
		throw std::invalid_argument("Only one color attachment is supported in OpenGL ES 2.0.");
	}
	attachTexture(context, GL_COLOR_ATTACHMENT0, t,level,layer);
}
void FBO::detachColorTexture(RenderingContext & context, uint32_t colorBufferId) {
	if(colorBufferId != 0) {
		throw std::invalid_argument("Only one color attachment is supported in OpenGL ES 2.0.");
	}
	detachTexture(context, GL_COLOR_ATTACHMENT0);
}
void FBO::attachDepthStencilTexture(RenderingContext & context, Texture * t,uint32_t level,uint32_t layer) {
	attachTexture(context, GL_DEPTH_ATTACHMENT, t,level,layer);
	attachTexture(context, GL_STENCIL_ATTACHMENT, t,level,layer);
}
void FBO::detachDepthStencilTexture(RenderingContext & context) {
	detachTexture(context, GL_DEPTH_ATTACHMENT);
	detachTexture(context, GL_STENCIL_ATTACHMENT);
}
void FBO::attachDepthTexture(RenderingContext & context, Texture * t,uint32_t level,uint32_t layer) {
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
	b = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) == GL_FRAMEBUFFER_COMPLETE_EXT;
#elif defined(LIB_GLESv2)
	b = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
#endif
	context.popFBO();
	return b;
}

const char * FBO::getStatusMessage(RenderingContext & context) {
	context.pushAndSetFBO(this);
#if defined(LIB_GL)
	const GLenum result = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
#elif defined(LIB_GLESv2)
	const GLenum result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
#endif
	context.popFBO();
#if defined(LIB_GL)
	switch(result) {
		case GL_FRAMEBUFFER_COMPLETE_EXT:
			return "Framebuffer complete.";
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
			return "[ERROR] Framebuffer incomplete: Attachment is NOT complete.";
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
			return "[ERROR] Framebuffer incomplete: No image is attached to FBO.";
		case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
			return "[ERROR] Framebuffer incomplete: Attached images have different dimensions.";
		case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
			return "[ERROR] Framebuffer incomplete: Color attached images have different internal formats.";
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
			return "[ERROR] Framebuffer incomplete: Draw buffer.";
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
			return "[ERROR] Framebuffer incomplete: Read buffer.";
		case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
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

#ifdef LIB_GL
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
	glDrawBuffers(static_cast<GLsizei>(number), buffers);
}
#endif /* LIB_GL */

}

/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "Texture.h"
#include "../GLHeader.h"
#include "../Helper.h"
#include "../RenderingContext/RenderingContext.h"
#include <Util/Graphics/PixelAccessor.h>

#ifdef LIB_GL
# ifdef __APPLE__
#  include <OpenGL/glu.h>
# else
#  include <GL/glu.h>
# endif
#endif /* LIB_GL */

namespace Rendering {

Texture::Format::Format():
	width(0), height(0), border(0),
	glTextureType(GL_TEXTURE_2D),
	glInternalFormat(GL_RGBA), glFormat(GL_RGBA),
	glDataType(GL_UNSIGNED_BYTE),
	wrapS(GL_REPEAT), wrapT(GL_REPEAT), wrapR(GL_REPEAT),
	magFilter(GL_LINEAR), minFilter(GL_LINEAR),
	compressed(false), imageSize(0) {
}

uint32_t Texture::Format::getPixelSize()const{
	uint32_t pixelSize = getGLTypeSize(glDataType);
	switch (glFormat){
		case GL_RGBA:
#ifdef LIB_GL
		case GL_BGRA:
#endif
			pixelSize*=4;
			break;
		case GL_RGB:
#ifdef LIB_GL
		case GL_BGR:
#endif
			pixelSize*=3;
			break;
		case GL_DEPTH_COMPONENT:
#ifdef LIB_GL
		case GL_RED:
		case GL_GREEN:
		case GL_BLUE:
		case GL_ALPHA:
		case GL_DEPTH_STENCIL_EXT:
#endif
			//pixelSize*=1;
			break;
		default:
			FAIL(); // not implemented... sorry...
	}
	return pixelSize;
}
// ----------------------------------------------------

//! [ctor]
Texture::Texture(const Format & _format):
		glId(0),format(_format),dataHasChanged(false),
		_pixelDataSize(format.getPixelSize()) {
}

//! [dtor]
Texture::~Texture() {
	removeGLData();
}

//! [dtor]
Texture * Texture::clone()const{
	auto t=new Texture(getFormat());
	if(localBitmap.isNotNull()){
		t->localBitmap=new Util::Bitmap( *localBitmap.get() );
		t->dataChanged();
	}
	t->setFileName(this->getFileName());
	return t;
}

bool Texture::createGLID(RenderingContext & context){
	
	int at;
	glGetIntegerv(GL_ACTIVE_TEXTURE, &at);
	
	// TODO!!! handle: dataHasChanged
	if (glId) {
		//INFO ("Recreating Texture!");
		if (isGLTextureValid()) {
			WARN("Recreating valid Texture!");
			removeGLData();
		}
	}
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	GET_GL_ERROR();
	glGenTextures(1,&glId);
	if(!glId)
		return false;
	GET_GL_ERROR();

	context.pushAndSetTexture(0,nullptr); // store and disable texture unit 0, so that we can use it without side effects.

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(format.glTextureType,glId);

	GET_GL_ERROR();
	// set parameters
	glTexParameteri(format.glTextureType,GL_TEXTURE_WRAP_S,format.wrapS);
	glTexParameteri(format.glTextureType,GL_TEXTURE_WRAP_T,format.wrapT);
#ifdef LIB_GL
	glTexParameteri(format.glTextureType,GL_TEXTURE_WRAP_R,format.wrapR);
#endif

	glTexParameteri(format.glTextureType,GL_TEXTURE_MAG_FILTER,format.magFilter);
	glTexParameteri(format.glTextureType,GL_TEXTURE_MIN_FILTER,format.minFilter);

	context.popTexture(0);
	
	glActiveTexture(at);

	return true;
}


bool Texture::uploadGLTexture(RenderingContext & context) {

	int at;
	glGetIntegerv(GL_ACTIVE_TEXTURE, &at);
	
	if(glId==0 && !createGLID(context))
		return false;

	context.pushAndSetTexture(0,nullptr); // store and disable texture unit 0, so that we can use it without side effects.

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(format.glTextureType,glId);

	switch (format.glTextureType) {
#ifdef LIB_GL
		case GL_TEXTURE_1D: {
			if (isMipmappingActive()) {
				gluBuild1DMipmaps(GL_TEXTURE_1D, format.glInternalFormat,
						static_cast<GLsizei>(format.width), format.glFormat, format.glDataType, getLocalData());
			} else {
				glTexImage1D(GL_TEXTURE_1D, 0, format.glInternalFormat,
						static_cast<GLsizei>(format.width), format.border, format.glFormat,
						format.glDataType, getLocalData());
			}
			break;
		}
#endif
		case GL_TEXTURE_2D: {
			if(format.compressed) {
				glCompressedTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLenum>(format.glInternalFormat),
									   static_cast<GLsizei>(format.width), static_cast<GLsizei>(format.height), 0,
									   static_cast<GLsizei>(format.imageSize), getLocalData());
				break;
			}

			glTexImage2D(GL_TEXTURE_2D, 0, format.glInternalFormat,
						 static_cast<GLsizei>(format.width), static_cast<GLsizei>(format.height), format.border,
						 format.glFormat, format.glDataType, getLocalData());

			if (isMipmappingActive()) {
#ifdef LIB_GL
				if(isExtensionSupported("GL_EXT_framebuffer_object")) { // glGenerateMipmapEXT is supported
					glGenerateMipmapEXT(GL_TEXTURE_2D);
				} else {
					gluBuild2DMipmaps(GL_TEXTURE_2D, format.glInternalFormat,
									  static_cast<GLsizei>(format.width), static_cast<GLsizei>(format.height),
									  format.glFormat, format.glDataType, getLocalData());
				}
#elif defined(LIB_GLESv2)
				glGenerateMipmap(GL_TEXTURE_2D);
#endif
			}
			break;
		}
		default:
			WARN("Unimplemented Texture Format.");
	}
	GET_GL_ERROR();
	dataHasChanged=false;

	context.popTexture(0);
	
	glActiveTexture(at);
	
	return true; // todo: return fals on error!
}

void Texture::_enable(RenderingContext & context) {
	if (!glId || dataHasChanged) {
		uploadGLTexture(context);
	}
	if (glId) {
#ifdef LIB_GL
		glEnable(format.glTextureType);
#endif /* LIB_GL */
		glBindTexture(format.glTextureType,glId);
	}
}

void Texture::allocateLocalData(){
	if(localBitmap.isNotNull()){
		WARN("Data already allocated");
		return;
	}
	using Util::PixelFormat;

	PixelFormat localFormat = PixelFormat::UNKNOWN;

#ifdef LIB_GL
	if (!format.compressed && (format.glTextureType==GL_TEXTURE_1D || format.glTextureType==GL_TEXTURE_2D)){
		if (format.glDataType==GL_FLOAT) {
			switch (format.glFormat){
				case GL_RGBA:
					localFormat = PixelFormat::RGBA_FLOAT;
					break;
				case GL_RGB:
					localFormat = PixelFormat::RGB_FLOAT;
					break;
				case GL_BGRA:
					localFormat = PixelFormat::BGRA_FLOAT;
					break;
				case GL_BGR:
					localFormat = PixelFormat::BGR_FLOAT;
					break;
				case GL_DEPTH_COMPONENT:
				case GL_RED:
					localFormat = PixelFormat(4, 0,PixelFormat::NONE,PixelFormat::NONE,PixelFormat::NONE);
					break;
				case GL_GREEN:
					localFormat = PixelFormat(4, PixelFormat::NONE,0,PixelFormat::NONE,PixelFormat::NONE);
					break;
				case GL_BLUE:
					localFormat = PixelFormat(4, PixelFormat::NONE,PixelFormat::NONE,0,PixelFormat::NONE);
					break;
				case GL_ALPHA:
					localFormat = PixelFormat(4, PixelFormat::NONE,PixelFormat::NONE,PixelFormat::NONE,0);
					break;
				default:
					break;
			}
		}else if (format.glDataType==GL_UNSIGNED_BYTE) {
			switch (format.glFormat){
				case GL_RGBA:
					localFormat = PixelFormat::RGBA;
					break;
				case GL_RGB:
					localFormat = PixelFormat::RGB;
					break;
				case GL_BGRA:
					localFormat = PixelFormat::BGRA;
					break;
				case GL_BGR:
					localFormat = PixelFormat::BGR;
					break;
				case GL_DEPTH_COMPONENT:
				case GL_RED:
					localFormat = PixelFormat(1, 0,PixelFormat::NONE,PixelFormat::NONE,PixelFormat::NONE);
					break;
				case GL_GREEN:
					localFormat = PixelFormat(1, PixelFormat::NONE,0,PixelFormat::NONE,PixelFormat::NONE);
					break;
				case GL_BLUE:
					localFormat = PixelFormat(1, PixelFormat::NONE,PixelFormat::NONE,0,PixelFormat::NONE);
					break;
				case GL_ALPHA:
					localFormat = PixelFormat(1, PixelFormat::NONE,PixelFormat::NONE,PixelFormat::NONE,0);
					break;
				default:
					break;
			}
		} else if (format.glDataType == GL_UNSIGNED_INT_24_8_EXT) {
			localFormat = PixelFormat::RGBA;
		}
	}
#else /* LIB_GL */
	if (!format.compressed && format.glTextureType == GL_TEXTURE_2D) {
		if (format.glDataType == GL_FLOAT) {
			switch (format.glFormat) {
				case GL_RGBA:
					localFormat = PixelFormat::RGBA_FLOAT;
					break;
				case GL_RGB:
					localFormat = PixelFormat::RGB_FLOAT;
					break;
				default:
					break;
			}
		} else if (format.glDataType == GL_UNSIGNED_BYTE) {
			switch (format.glFormat) {
				case GL_RGBA:
					localFormat = PixelFormat::RGBA;
					break;
				case GL_RGB:
					localFormat = PixelFormat::RGB;
					break;
				default:
					break;
			}
		}
	}
#endif /* LIB_GL */

	// No default format found...
	if(localFormat == PixelFormat::UNKNOWN) {
		localBitmap = new Util::Bitmap(getWidth(), getHeight(), static_cast<std::size_t>(format.getDataSize()));
	}else {
		localBitmap = new Util::Bitmap(getWidth(), getHeight(), localFormat);
	}

}

void Texture::_disable() {
#ifdef LIB_GL
	if (glId) {
		glDisable(format.glTextureType);
	}
#endif /* LIB_GL */
}

bool Texture::isGLTextureValid()const {
	return glId==0?false: (glIsTexture(glId)==GL_TRUE) ;
}

bool Texture::isGLTextureResident()const {
#ifdef LIB_GL
	GLboolean b;
	glAreTexturesResident(1,&glId,&b);
	return b==GL_TRUE;
#else
	WARN("isGLTextureResident not supported.");
	return true;
#endif
}

void Texture::removeLocalData(){
	localBitmap=nullptr;
}

void Texture::removeGLData(){
	if(glId)
		glDeleteTextures(1,&glId);
	glId=0;
}

bool Texture::downloadGLTexture(RenderingContext & context) {
#ifdef LIB_GL
	if(!glId){
		WARN("No glTexture available.");
		return false;
	}
	if(localBitmap.isNull()){
		allocateLocalData();
	}
	context.pushAndSetTexture(0,this);
	glGetTexImage(format.glTextureType, 0, format.glFormat, format.glDataType, getLocalData());
	context.popTexture(0);
	return true;
#else
	WARN("downloadGLTexture not supported.");
	return false;
#endif
}

uint8_t * Texture::getLocalData() {
	return localBitmap.isNull() ? nullptr : localBitmap->data();
}

const uint8_t * Texture::getLocalData()const {
	return localBitmap.isNull() ? nullptr : localBitmap->data();
}

uint8_t * Texture::openLocalData(RenderingContext & context){
	if(localBitmap.isNull()){
		allocateLocalData();
		if(glId!=0)
			downloadGLTexture(context);
	}
	return localBitmap->data();
}

bool Texture::isMipmappingActive() const {
	return 	format.minFilter == GL_NEAREST_MIPMAP_NEAREST ||
			format.minFilter == GL_LINEAR_MIPMAP_NEAREST ||
			format.minFilter == GL_NEAREST_MIPMAP_LINEAR ||
			format.minFilter == GL_LINEAR_MIPMAP_LINEAR;
}

}

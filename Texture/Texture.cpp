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
#include <Util/Graphics/Bitmap.h>
#include <Util/Graphics/PixelFormat.h>
#include <Util/Macros.h>
#include <Util/References.h>
#include <cstddef>
#include <iostream>

#ifdef LIB_GL
# ifdef __APPLE__
#  include <OpenGL/glu.h>
# else
#  include <GL/glu.h>
# endif
#endif /* LIB_GL */

namespace Rendering {

Texture::Format::Format():
		sizeX(0), sizeY(0), numLayers(1), 
		glTextureType(GL_TEXTURE_2D),
		glInternalFormat(GL_RGBA), glFormat(GL_RGBA),
		compressed(false), compressedImageSize(0),
		glDataType(GL_UNSIGNED_BYTE),
		glWrapS(GL_REPEAT), glWrapT(GL_REPEAT), glWrapR(GL_REPEAT),
		glMagFilter(GL_LINEAR), glMinFilter(GL_LINEAR),
		autoCreateMipmaps(true) {
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
Texture::Texture(Format _format):
		glId(0),format(std::move(_format)),dataHasChanged(true),
		_pixelDataSize(format.getPixelSize()) {
	// checkFormatConsistency
}

//! [dtor]
Texture::~Texture() {
	removeGLData();
}
//
//std::pair<Util::Reference<Util::Bitmap>,bool>& Texture::checkFormatConsistency(imageId_t subImage){
//	const auto it = localBitmaps.find(subImage);
//	
//	if(it==localBitmaps.end()){			
////		const uint8_t level = std::get<0>(subImage);
//		const uint32_t layer = std::get<1>(subImage);
//		const uint8_t face = std::get<2>(subImage);
//		
//		switch( format.glTextureType ){
//			case GL_TEXTURE_1D:
//			case GL_TEXTURE_2D:
//				if(layer>0||face>0)
//					throw std::runtime_error("Texture::accessSubImageEntry: No faces or layers supported for given texture type.");
//				break;
//#ifdef LIB_GL
//			case GL_TEXTURE_3D:
//				if(face>0)
//					throw std::runtime_error("Texture::accessSubImageEntry: No faces supported for 3d-textures.");
//				break;
//			case GL_TEXTURE_CUBE_MAP:
//				if(layer>0)
//					throw std::runtime_error("Texture::accessSubImageEntry: No layer supported for cube-textures.");
//				if(face>5)
//					throw std::runtime_error("Texture::accessSubImageEntry: Invalid cube-texture face.");
//				break;
//#endif
//			default:
//				//! \todo Implement array textures, 3d textures, ...
//				throw std::runtime_error("Texture::accessSubImageEntry: unsupported texture type.");
//		}
//		localBitmaps[subImage] = std::make_pair(nullptr,true);
//		dataHasChanged = true;
//		return localBitmaps[subImage];
//	}
//	return it->second;
//}



void Texture::_createGLID(RenderingContext & context){
	// TODO!!! handle: dataHasChanged
	if(glId) {
		//INFO ("Recreating Texture!");
		if(isGLTextureValid()) {
			WARN("Recreating valid Texture!");
			removeGLData();
		}
	}
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	GET_GL_ERROR();
	glGenTextures(1,&glId);
	if(!glId){
		GET_GL_ERROR();
		throw std::runtime_error("Texture::_createGLID: Could not create texture.");
	}

	GLint activeTexture;
	glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);
	context.pushAndSetTexture(0,nullptr); // store and disable texture unit 0, so that we can use it without side effects.

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(format.glTextureType,glId);

	GET_GL_ERROR();
	// set parameters
	glTexParameteri(format.glTextureType,GL_TEXTURE_WRAP_S,format.glWrapS);
	glTexParameteri(format.glTextureType,GL_TEXTURE_WRAP_T,format.glWrapT);
#ifdef LIB_GL
	glTexParameteri(format.glTextureType,GL_TEXTURE_WRAP_R,format.glWrapR);
#endif

	glTexParameteri(format.glTextureType,GL_TEXTURE_MAG_FILTER,format.glMagFilter);
	glTexParameteri(format.glTextureType,GL_TEXTURE_MIN_FILTER,format.glMinFilter);
	context.popTexture(0);
	
	glActiveTexture(activeTexture);
	GET_GL_ERROR();
}


void Texture::createMipMaps(RenderingContext & context) {
	GLint activeTexture;
	glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);
	
	if(!glId)
		_createGLID(context);


	context.pushAndSetTexture(0,nullptr); // store and disable texture unit 0, so that we can use it without side effects.

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(format.glTextureType,glId);


	switch (format.glTextureType) {
		case GL_TEXTURE_2D: {
#ifdef LIB_GL
			if(isExtensionSupported("GL_EXT_framebuffer_object")) { // glGenerateMipmapEXT is supported
				glGenerateMipmapEXT(GL_TEXTURE_2D);
			} else {
				gluBuild2DMipmaps(GL_TEXTURE_2D, static_cast<GLenum>(format.glInternalFormat),
									static_cast<GLsizei>(getWidth()), 
									static_cast<GLsizei>(getHeight()),
									format.glFormat, format.glDataType, getLocalData());
			}
#elif defined(LIB_GLESv2)
			glGenerateMipmap(GL_TEXTURE_2D);
#endif
			break;
		}
		default:{
			context.popTexture(0);
			glActiveTexture(activeTexture);
			throw std::runtime_error("Texture::createMipMaps: unsupported texture type.");
		}
	}
	GET_GL_ERROR();
	context.popTexture(0);
	glActiveTexture(activeTexture);
}
	
void Texture::_uploadGLTexture(RenderingContext & context) {
	GLint activeTexture;
	glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);
	
	if(!glId)
		_createGLID(context);

	dataHasChanged = false;

	context.pushAndSetTexture(0,nullptr); // store and disable texture unit 0, so that we can use it without side effects.

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(format.glTextureType,glId);


	switch (format.glTextureType) {
#ifdef LIB_GL
	//! \todo add cube map support and 3d-texture support

		case GL_TEXTURE_1D: {
			if(format.autoCreateMipmaps) {
				gluBuild1DMipmaps(GL_TEXTURE_1D, format.glInternalFormat,
						static_cast<GLsizei>(getWidth()), format.glFormat, 
						format.glDataType, getLocalData());
			} else {
				glTexImage1D(GL_TEXTURE_1D, 0, format.glInternalFormat,
						static_cast<GLsizei>(getWidth()), 0, format.glFormat,
						format.glDataType, getLocalData());
			}
			break;
		}
#endif
		case GL_TEXTURE_2D: {
			if(format.compressed) {
				glCompressedTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLenum>(format.glInternalFormat),
										static_cast<GLsizei>(getWidth()), 
										static_cast<GLsizei>(getHeight()), 0,
										static_cast<GLsizei>(format.compressedImageSize), 
										getLocalData());
				break;
			}

			glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLenum>(format.glInternalFormat),
										static_cast<GLsizei>(getWidth()), 
										static_cast<GLsizei>(getHeight()), 0,
										format.glFormat, format.glDataType, getLocalData());

			if(format.autoCreateMipmaps) {
#ifdef LIB_GL
				if(isExtensionSupported("GL_EXT_framebuffer_object")) { // glGenerateMipmapEXT is supported
					glGenerateMipmapEXT(GL_TEXTURE_2D);
				} else {
					gluBuild2DMipmaps(GL_TEXTURE_2D, static_cast<GLenum>(format.glInternalFormat),
										static_cast<GLsizei>(getWidth()), 
										static_cast<GLsizei>(getHeight()),
										format.glFormat, format.glDataType, getLocalData());
				}
#elif defined(LIB_GLESv2)
				glGenerateMipmap(GL_TEXTURE_2D);
#endif
			}
			break;
		}
		default:{
			context.popTexture(0);
			glActiveTexture(activeTexture);
			throw std::runtime_error("Texture::_uploadGLTexture: unsupported texture type.");
		}
	}
	GET_GL_ERROR();
	context.popTexture(0);
	glActiveTexture(activeTexture);
}

void Texture::allocateLocalData(){
	if(localBitmap.isNotNull()){
		WARN("Data already allocated");
		return;
	}
	using Util::PixelFormat;

	PixelFormat localFormat = PixelFormat::UNKNOWN;

#ifdef LIB_GL
	if(!format.compressed && (format.glTextureType==GL_TEXTURE_1D || format.glTextureType==GL_TEXTURE_2D)){
		if(format.glDataType==GL_FLOAT) {
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
		}else if(format.glDataType==GL_UNSIGNED_BYTE) {
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
		} else if(format.glDataType == GL_UNSIGNED_INT_24_8_EXT) {
			localFormat = PixelFormat::RGBA;
		}
	}
#else /* LIB_GL */
	if(!format.compressed && format.glTextureType == GL_TEXTURE_2D) {
		if(format.glDataType == GL_FLOAT) {
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
		} else if(format.glDataType == GL_UNSIGNED_BYTE) {
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
		localBitmap = new Util::Bitmap(getWidth(), getHeight()*getNumLayers(), static_cast<std::size_t>(format.getDataSize()));
	}else {
		localBitmap = new Util::Bitmap(getWidth(), getHeight()*getNumLayers(), localFormat);
	}

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

void Texture::removeGLData(){
	if(glId)
		glDeleteTextures(1,&glId);
	glId=0;
}

void Texture::downloadGLTexture(RenderingContext & context) {
#ifdef LIB_GL
	if(!glId){
		WARN("No glTexture available.");
		return;
	}
	dataHasChanged = false;

	if(!localBitmap)
		allocateLocalData();

	context.pushAndSetTexture(0,this);
//	GL_TEXTURE_CUBE_MAP_POSITIVE_X
	switch( format.glTextureType ){
		case GL_TEXTURE_1D:
		case GL_TEXTURE_2D:
			glGetTexImage(format.glTextureType, 0, format.glFormat, format.glDataType, getLocalData());
			break;
//		case GL_TEXTURE_CUBE_MAP:
//			glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X+face, 0, format.glFormat, format.glDataType, getLocalData(subImage));
//			glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X+face, 0, format.glFormat, format.glDataType, getLocalData(subImage));
//			glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X+face, 0, format.glFormat, format.glDataType, getLocalData(subImage));
//			glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X+face, 0, format.glFormat, format.glDataType, getLocalData(subImage));
//			glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X+face, 0, format.glFormat, format.glDataType, getLocalData(subImage));
//			break;
		default:
			throw std::runtime_error("Texture::downloadGLTexture: unsupported texture type.");
	}

	context.popTexture(0);
#else
	WARN("downloadGLTexture not supported.");
#endif
}

uint8_t * Texture::getLocalData()							{	return localBitmap ? localBitmap->data() : nullptr;	}
const uint8_t * Texture::getLocalData() const				{	return localBitmap ? localBitmap->data() : nullptr;	}

uint8_t * Texture::openLocalData(RenderingContext & context){
	if(!localBitmap){
		allocateLocalData();
		downloadGLTexture(context);
	}
	return getLocalData();
}

}

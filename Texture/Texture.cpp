/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2014 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "Texture.h"
#include "../GLHeader.h"
#include "../Memory/BufferObject.h"
#include "../Helper.h"
#include "../RenderingContext/RenderingContext.h"
#include "TextureUtils.h"
#include <Util/Graphics/Bitmap.h>
#include <Util/Graphics/PixelFormat.h>
#include <Util/Macros.h>
#include <Util/References.h>
#include <Util/Graphics/PixelAccessor.h>
#include <Util/StringUtils.h>
#include <cstddef>
#include <iostream>
#include <cmath>


namespace Rendering {



Texture::Format::Format():
		sizeX(0), sizeY(0), numLayers(1),
		glTextureType(GL_TEXTURE_2D),
		compressedImageSize(0),
		glWrapS(GL_REPEAT), glWrapT(GL_REPEAT), glWrapR(GL_REPEAT),
		linearMinFilter(true),linearMagFilter(true) {
}

uint32_t Texture::Format::getPixelSize()const{
	uint32_t pixelSize = getGLTypeSize(pixelFormat.glLocalDataType);
	switch(pixelFormat.glLocalDataFormat){
		case GL_RG:
		case GL_RG_INTEGER:
			pixelSize*=4;
			break;
		case GL_RGBA:
		case GL_BGRA:
		case GL_RGBA_INTEGER:
			pixelSize*=4;
			break;
		case GL_RGB:
		case GL_BGR:
		case GL_RGB_INTEGER:
			pixelSize*=3;
			break;
		case GL_DEPTH_COMPONENT:
		case GL_RED:
		case GL_RED_INTEGER:
		case GL_GREEN:
		case GL_BLUE:
		case GL_ALPHA:
		case GL_DEPTH_STENCIL_EXT:
			//pixelSize*=1;
			break;
		default:
			throw std::runtime_error("Format::getPixelSize: Unsupported format.");
	}
	return pixelSize;
}
// ----------------------------------------------------

//! [ctor]
Texture::Texture(Format _format):
		glId(0),format(std::move(_format)),dataHasChanged(true),hasMipmaps(false),mipmapCreationIsPlanned(false),
		_pixelDataSize(format.getPixelSize()) {
	switch(format.glTextureType){
		case GL_TEXTURE_1D:
			tType = TextureType::TEXTURE_1D;
			if(format.numLayers!=1 || format.sizeY!=1 )
				throw std::logic_error("Texture: TEXTURE_1D expects numLayers == 1 && sizeY == 1.");
			break;
		case GL_TEXTURE_1D_ARRAY:
			tType = TextureType::TEXTURE_1D_ARRAY;
			break;
		case GL_TEXTURE_2D:
			tType = TextureType::TEXTURE_2D;
			if(format.numLayers!=1)
				throw std::logic_error("Texture: TEXTURE_2D expects numLayers == 1.");
			break;
		case GL_TEXTURE_2D_ARRAY:
			tType = TextureType::TEXTURE_2D_ARRAY;
			break;
		case GL_TEXTURE_3D:
			tType = TextureType::TEXTURE_3D;
			break;
		case GL_TEXTURE_CUBE_MAP:
			tType = TextureType::TEXTURE_CUBE_MAP;
			if(format.numLayers!=6)
				throw std::logic_error("Texture: TEXTURE_CUBE_MAP expects numLayers == 6.");
			break;
		case GL_TEXTURE_CUBE_MAP_ARRAY:
			tType = TextureType::TEXTURE_CUBE_MAP_ARRAY;
			if( (format.numLayers%6) !=0)
				throw std::logic_error("Texture: TEXTURE_CUBE_MAP expects (numLayers%6) == 0.");
			break;
		case GL_TEXTURE_BUFFER:
			if( format.numLayers != 1 || format.sizeY!=1)
				throw std::logic_error("Texture: TEXTURE_BUFFER expects numLayers == 1 && sizeY == 1.");
			tType = TextureType::TEXTURE_BUFFER;
			bufferObject.reset( new BufferObject );
			break;
		case GL_TEXTURE_2D_MULTISAMPLE:
			tType = TextureType::TEXTURE_2D_MULTISAMPLE;
			if(format.numLayers!=1)
				throw std::logic_error("Texture: TEXTURE_2D_MULTISAMPLE expects numLayers == 1.");
			break;
		default:
			throw std::runtime_error("Texture: Unsupported texture type.");
	}
	// checkFormatConsistency
}

//! [dtor]
Texture::~Texture() {
	removeGLData();
}

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
	//glGenTextures(1,&glId);
	glCreateTextures(format.glTextureType, 1, &glId);
	if(!glId){
		GET_GL_ERROR();
		throw std::runtime_error("Texture::_createGLID: Could not create texture.");
	}
	
	GET_GL_ERROR();
	if( tType!=TextureType::TEXTURE_BUFFER && tType!=TextureType::TEXTURE_2D_MULTISAMPLE ){
		// set parameters
		// TODO: move to separate sampler
		glTextureParameteri(glId,GL_TEXTURE_WRAP_S,format.glWrapS);
		glTextureParameteri(glId,GL_TEXTURE_WRAP_T,format.glWrapT);
		glTextureParameteri(glId,GL_TEXTURE_WRAP_R,format.glWrapR);
		glTextureParameteri(glId,GL_TEXTURE_MAG_FILTER,format.linearMagFilter ? GL_LINEAR : GL_NEAREST);
		if(mipmapCreationIsPlanned) {
			if(format.pixelFormat.glLocalDataType == GL_UNSIGNED_INT || format.pixelFormat.glLocalDataType == GL_INT) {
			  glTexParameteri(format.glTextureType,GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
			} else {
			  glTexParameteri(format.glTextureType,GL_TEXTURE_MIN_FILTER,format.linearMinFilter ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST);
			}
		} else {
			glTextureParameteri(glId,GL_TEXTURE_MIN_FILTER,format.linearMinFilter ? GL_LINEAR : GL_NEAREST);
		}
	}
	GET_GL_ERROR();
}


void Texture::createMipmaps(RenderingContext & context) {
	mipmapCreationIsPlanned = true;
	
	if(!glId || dataHasChanged)
		_uploadGLTexture(context);

	mipmapCreationIsPlanned = false;
	glGenerateTextureMipmap(glId);
	hasMipmaps = true;
	GET_GL_ERROR();
}

void Texture::_uploadGLTexture(RenderingContext & context) {
	if(!glId)
		_createGLID(context);

	dataHasChanged = false;
	int levels = mipmapCreationIsPlanned ? (std::log2(std::max(getWidth(), getHeight())) + 1) : 1;

	switch(tType) {
	//! \todo add cube map support and 3d-texture support

		case TextureType::TEXTURE_1D: {
			glTextureStorage1D(glId, levels, format.pixelFormat.glInternalFormat, getWidth());
			if(getLocalData())
				glTextureSubImage1D(glId, 0, 0, getWidth(), format.pixelFormat.glLocalDataFormat, format.pixelFormat.glLocalDataType, getLocalData());
			GET_GL_ERROR();
			break;
		}
		case TextureType::TEXTURE_2D: {
			glTextureStorage2D(glId, levels, format.pixelFormat.glInternalFormat, getWidth(), getHeight());
			if(getLocalData())
				glTextureSubImage2D(glId, 0, 0, 0, getWidth(), getHeight(), format.pixelFormat.glLocalDataFormat, format.pixelFormat.glLocalDataType, getLocalData());
			GET_GL_ERROR();
			break;
		}
		case TextureType::TEXTURE_CUBE_MAP:{		
			glTextureStorage3D(glId, levels, format.pixelFormat.glInternalFormat, getWidth(), getHeight(), 6);
			if(getLocalData())
				glTextureSubImage3D(glId, 0, 0, 0, 0, getWidth(), getWidth(), 6, format.pixelFormat.glLocalDataFormat, format.pixelFormat.glLocalDataType, getLocalData());
			/*Util::Reference<Util::PixelAccessor> pa =  Util::PixelAccessor::create(getLocalBitmap());
			if(pa){ // local data available?
				for(uint_fast8_t layer =0; layer < 6; layer++){
					glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + layer, level, static_cast<GLint>(format.pixelFormat.glInternalFormat),
								width, height, 0,
								static_cast<GLenum>(format.pixelFormat.glLocalDataFormat), 
								static_cast<GLenum>(format.pixelFormat.glLocalDataType), pa->_ptr<uint8_t>(0, height * layer));
				}
			}
			else{ // -> just allocate gpu data.
				for(uint_fast8_t layer =0; layer < 6; ++layer){
					glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + layer, level, static_cast<GLint>(format.pixelFormat.glInternalFormat),
								width, height, 0,
								static_cast<GLenum>(format.pixelFormat.glLocalDataFormat), 
								static_cast<GLenum>(format.pixelFormat.glLocalDataType), nullptr);
				}
			}*/
			break;
		}
		case  TextureType::TEXTURE_BUFFER:{
			if(!bufferObject->isValid()) {
				bufferObject->allocate(getDataSize(), BufferObject::FLAGS_STATIC, getLocalData());
			} else if(getLocalData()) {
				bufferObject->upload(getLocalData(), getDataSize());
			}
			
			// special case:the used internalFormat in TextureUtils is not applicable here
			/*const auto& pixelFormat = getFormat().pixelFormat;
			GLenum format = pixelFormat.glInternalFormat;
			if(pixelFormat.glLocalDataType==GL_BYTE || pixelFormat.glLocalDataType==GL_UNSIGNED_BYTE){
				if(pixelFormat.glInternalFormat==GL_RED){
					format = GL_R8;
				}else if(pixelFormat.glInternalFormat==GL_RG){
					format = GL_RG8;
				}else if(pixelFormat.glInternalFormat==GL_RGB){
					format = GL_RGB8; // not supported by opengl!
				}else if(pixelFormat.glInternalFormat==GL_RGBA){
					format = GL_RGBA8;
				}
			}*/
			// bind the buffer to the texture
			glTextureBuffer( glId, format.pixelFormat.glInternalFormat, bufferObject->getGLId() );
			break;
		
		}
		case  TextureType::TEXTURE_1D_ARRAY:{
			glTextureStorage2D(glId, levels, format.pixelFormat.glInternalFormat, getWidth(), getNumLayers());
			if(getLocalData())
				glTextureSubImage2D(glId, 0, 0, 0, getWidth(), getNumLayers(), format.pixelFormat.glLocalDataFormat, format.pixelFormat.glLocalDataType, getLocalData());
			GET_GL_ERROR();
			break;
		}
		case  TextureType::TEXTURE_2D_ARRAY:
		case  TextureType::TEXTURE_3D:
		case  TextureType::TEXTURE_CUBE_MAP_ARRAY:{
			glTextureStorage3D(glId, levels, format.pixelFormat.glInternalFormat, getWidth(), getHeight(), getNumLayers());
			if(getLocalData())
				glTextureSubImage3D(glId, 0, 0, 0, 0, getWidth(), getWidth(), getNumLayers(), format.pixelFormat.glLocalDataFormat, format.pixelFormat.glLocalDataType, getLocalData());
			GET_GL_ERROR();
			break;
		}
		case TextureType::TEXTURE_2D_MULTISAMPLE: {
			glTextureStorage2DMultisample(glId, format.numSamples, format.pixelFormat.glInternalFormat, getWidth(), getHeight(), false);
			if(getLocalData())
				glTextureSubImage2D(glId, 0, 0, 0, getWidth(), getHeight(), format.pixelFormat.glLocalDataFormat, format.pixelFormat.glLocalDataType, getLocalData());
			GET_GL_ERROR();
			break;
		}
		default:{
			throw std::runtime_error("Texture::_uploadGLTexture: Unsupported texture type.");
		}
	}
}

void Texture::allocateLocalData(){
	if(localBitmap.isNotNull()){
		WARN("Texture::allocateLocalData: Data already allocated");
		return;
	}
	if( format.pixelFormat.compressed ){ // download raw data
		localBitmap = new Util::Bitmap(getWidth(), getHeight()*getNumLayers(), static_cast<std::size_t>(format.getDataSize()));
	}else{
		const auto localFormat = TextureUtils::glPixelFormatToPixelFormat( format.pixelFormat );
		if(localFormat == Util::PixelFormat::UNKNOWN) {
			WARN("Texture::allocateLocalData: Unsupported pixel format.");
			localBitmap = nullptr;
		}else{
			localBitmap = new Util::Bitmap(getWidth(), getHeight()*getNumLayers(), localFormat);
		}
	}
}

bool Texture::isGLTextureValid()const {
	return glId==0?false: (glIsTexture(glId)==GL_TRUE) ;
}

bool Texture::isGLTextureResident()const {
	GLboolean b;
	glAreTexturesResident(1,&glId,&b);
	return b==GL_TRUE;
}

void Texture::removeGLData(){
	if(glId)
		glDeleteTextures(1,&glId);
	glId=0;
}

void Texture::clearGLData(const Util::Color4f& color) {
	static const bool clearSupported = isExtensionSupported("GL_VERSION_4_4");
	if(!clearSupported){
		WARN("clearGLData: Requires at least OpenGL 4.4 to work.");
		return;
	}
	if(!isGLTextureValid())
		return;
	
	int maxLevel = hasMipmaps ? std::log2(std::max(getWidth(), getHeight())) : 0;
	std::vector<uint8_t> data;
	
	if(format.pixelFormat.glLocalDataType == GL_FLOAT) {
		data.resize(sizeof(float)*4);
		std::copy(reinterpret_cast<const uint8_t*>(color.data()), reinterpret_cast<const uint8_t*>(color.data()+4), data.data());
	} else if(format.pixelFormat.glLocalDataType == GL_UNSIGNED_INT || format.pixelFormat.glLocalDataType == GL_INT) {
		data.resize(sizeof(int32_t)*4);
		std::vector<int32_t> values = {static_cast<int32_t>(color.r()),static_cast<int32_t>(color.g()), static_cast<int32_t>(color.b()), static_cast<int32_t>(color.a())};
		std::copy(reinterpret_cast<const uint8_t*>(values.data()), reinterpret_cast<const uint8_t*>(values.data()+4), data.data());
	} else if(format.pixelFormat.glLocalDataType == GL_UNSIGNED_BYTE || format.pixelFormat.glLocalDataType == GL_BYTE) {
		data.resize(4);
		Util::Color4ub colorb(color);
		std::copy(colorb.data(), colorb.data()+4, data.data());
	}

	for(int level=0; level<=maxLevel; ++level)
		glClearTexImage(glId, level, format.pixelFormat.glLocalDataFormat, format.pixelFormat.glLocalDataType, data.empty() ? nullptr : data.data());
}

void Texture::downloadGLTexture(RenderingContext & context, uint8_t* target, uint32_t level) {
	if(!glId){
		WARN("downloadGLTexture: No glTexture available.");
		return;
	}
	dataHasChanged = false;
	
	if(level>0 && target==nullptr) {
		WARN("downloadGLTexture: Cannot download mip level without given target.");
		return;
	}
	
	if(level>0 && !hasMipmaps) {
		WARN("downloadGLTexture: Texture has no mip levels.");
		return;
	}

	size_t dataSize = getDataSize() >> (level*2);
	if(!target) {
		if(localBitmap.isNull())
			allocateLocalData();
		target = getLocalData();
	}

	context.applyChanges();
	switch( tType ){
		case  TextureType::TEXTURE_1D:
		case  TextureType::TEXTURE_2D:
		case  TextureType::TEXTURE_1D_ARRAY:
		case  TextureType::TEXTURE_2D_ARRAY:
		case  TextureType::TEXTURE_3D:
		case  TextureType::TEXTURE_CUBE_MAP:
			glGetTextureImage(glId, level, format.pixelFormat.glLocalDataFormat, format.pixelFormat.glLocalDataType, dataSize, target);
			break;
		/*case  TextureType::TEXTURE_CUBE_MAP:{
			Util::Reference<Util::PixelAccessor> pa =  Util::PixelAccessor::create(getLocalBitmap());
			if(pa){
				for(uint_fast8_t layer = 0; layer < 6; ++layer){
					glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + layer, 0, 
								static_cast<GLenum>(format.pixelFormat.glLocalDataFormat), static_cast<GLenum>(format.pixelFormat.glLocalDataType), 
								pa->_ptr<uint8_t>(0, getHeight() * layer));
				}
			}
			else
				throw std::runtime_error("Texture::downloadGLTexture: unsupported pixel format.");
			break;
		}*/
		case TextureType::TEXTURE_BUFFER:{
			bufferObject->download(target, dataSize);
			break;
		}
		case  TextureType::TEXTURE_CUBE_MAP_ARRAY:
		default:
			throw std::runtime_error("Texture::downloadGLTexture: (currently) unsupported texture type.");
	}
}

uint8_t * Texture::getLocalData()							{	return localBitmap ? localBitmap->data() : nullptr;	}
const uint8_t * Texture::getLocalData() const				{	return localBitmap ? localBitmap->data() : nullptr;	}

uint8_t * Texture::openLocalData(RenderingContext & context){
	if(!localBitmap){
		allocateLocalData();
		if(getGLId()!=0)
			downloadGLTexture(context);
	}
	return getLocalData();
}

void Texture::_setGLId(uint32_t _glId) {
	removeGLData();
	glId = _glId;
	if(!isGLTextureValid()) {
		WARN("Texture::_setGLId: The given id is not a valid texture " + Util::StringUtils::toString(_glId));
		glId = 0;
	}	
}

}

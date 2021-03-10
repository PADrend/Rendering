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
#include "../BufferObject.h"
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
#ifdef LIB_GL
		case GL_RG:
		case GL_RG_INTEGER:
			pixelSize*=2;
			break;
#endif

		case GL_RGBA:
#ifdef LIB_GL
		case GL_BGRA:
		case GL_RGBA_INTEGER:
#endif
			pixelSize*=4;
			break;
		case GL_RGB:
#ifdef LIB_GL
		case GL_BGR:
		case GL_RGB_INTEGER:
#endif
			pixelSize*=3;
			break;
		case GL_DEPTH_COMPONENT:
#ifdef LIB_GL
		case GL_RED:
		case GL_RED_INTEGER:
		case GL_GREEN:
		case GL_BLUE:
		case GL_ALPHA:
		case GL_DEPTH_STENCIL_EXT:
#endif
			//pixelSize*=1;
			break;
		default:
			throw std::runtime_error("Format::getPixelSize: Unsupported format " + std::to_string(pixelFormat.glLocalDataFormat) + ".");
	}
	return pixelSize;
}
// ----------------------------------------------------

//! [ctor]
Texture::Texture(Format _format):
		glId(0),format(std::move(_format)),dataHasChanged(true),hasMipmaps(false),mipmapCreationIsPlanned(false),
		_pixelDataSize(format.getPixelSize()) {
	switch(format.glTextureType){
#if defined(LIB_GL)
		case GL_TEXTURE_1D:
			tType = TextureType::TEXTURE_1D;
			if(format.numLayers!=1 || format.sizeY!=1 )
				throw std::logic_error("Texture: TEXTURE_1D expects numLayers == 1 && sizeY == 1.");
			break;
		case GL_TEXTURE_1D_ARRAY:
			tType = TextureType::TEXTURE_1D_ARRAY;
			break;
#endif
		case GL_TEXTURE_2D:
			tType = TextureType::TEXTURE_2D;
			if(format.numLayers!=1)
				throw std::logic_error("Texture: TEXTURE_2D expects numLayers == 1.");
			break;
#if defined(LIB_GL)
		case GL_TEXTURE_2D_ARRAY:
			tType = TextureType::TEXTURE_2D_ARRAY;
			break;
		case GL_TEXTURE_3D:
			tType = TextureType::TEXTURE_3D;
			break;
#endif
		case GL_TEXTURE_CUBE_MAP:
			tType = TextureType::TEXTURE_CUBE_MAP;
			if(format.numLayers!=6)
				throw std::logic_error("Texture: TEXTURE_CUBE_MAP expects numLayers == 6.");
			break;
#if defined(LIB_GL)
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
#endif
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
	if( tType!=TextureType::TEXTURE_BUFFER && tType!=TextureType::TEXTURE_2D_MULTISAMPLE ){
		// set parameters
		glTexParameteri(format.glTextureType,GL_TEXTURE_WRAP_S,format.glWrapS);
		glTexParameteri(format.glTextureType,GL_TEXTURE_WRAP_T,format.glWrapT);
	#ifdef LIB_GL
		glTexParameteri(format.glTextureType,GL_TEXTURE_WRAP_R,format.glWrapR);
	#endif

		glTexParameteri(format.glTextureType,GL_TEXTURE_MAG_FILTER,format.linearMagFilter ? GL_LINEAR : GL_NEAREST);
		glTexParameteri(format.glTextureType,GL_TEXTURE_MIN_FILTER,format.linearMinFilter ? GL_LINEAR : GL_NEAREST);
	}
	context.popTexture(0);

	glActiveTexture(activeTexture);
	GET_GL_ERROR();
}


void Texture::createMipmaps(RenderingContext & context) {
	if(!glId || dataHasChanged)
		_uploadGLTexture(context);

	mipmapCreationIsPlanned = false;
	static const bool mipmapCreationSupported = isExtensionSupported("GL_EXT_framebuffer_object");
	if(mipmapCreationSupported){

		GLint activeTexture;
		glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);
		context.pushAndSetTexture(0,nullptr); // store and disable texture unit 0, so that we can use it without side effects.

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(format.glTextureType,glId);
		GET_GL_ERROR();

		if(format.pixelFormat.glLocalDataType == GL_UNSIGNED_INT || format.pixelFormat.glLocalDataType == GL_INT) {
			// for integer textures glGenerateMipmap is prohibited, therefore just allocate the storage
			int maxLevel = static_cast<int>(std::log2(std::max(getWidth(), getHeight())));
			for(int level=1; level<=maxLevel; ++level)
				_uploadGLTexture(context, level);
		} else {
			#ifdef LIB_GL
				glGenerateMipmap(format.glTextureType);
			#elif defined(LIB_GLESv2)
				glGenerateMipmap(GL_TEXTURE_2D);
			#endif
		}

		hasMipmaps = true;
		if(format.pixelFormat.glLocalDataType == GL_UNSIGNED_INT || format.pixelFormat.glLocalDataType == GL_INT) {
		  glTexParameteri(format.glTextureType,GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		} else {
		  glTexParameteri(format.glTextureType,GL_TEXTURE_MIN_FILTER,format.linearMinFilter ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST);
		}

		GET_GL_ERROR();
		context.popTexture(0);
		glActiveTexture(activeTexture);
	}
}

void Texture::_uploadGLTexture(RenderingContext & context, int level/*=0*/) {
	GLint activeTexture;
	glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);

	if(!glId)
		_createGLID(context);

	dataHasChanged = false;

	context.pushAndSetTexture(0,nullptr); // store and disable texture unit 0, so that we can use it without side effects.
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(format.glTextureType,glId);
	auto width = std::max(1, static_cast<GLsizei>(getWidth()) >> level);
	auto height = std::max(1, static_cast<GLsizei>(getHeight()) >> level);
	auto depth = tType == TextureType::TEXTURE_3D ? std::max(1, static_cast<GLsizei>(getNumLayers()) >> level) : getNumLayers();

	switch(tType) {
#ifdef LIB_GL
	//! \todo add cube map support and 3d-texture support

		case TextureType::TEXTURE_1D: {
			glTexImage1D(GL_TEXTURE_1D, level, static_cast<GLint>(format.pixelFormat.glInternalFormat),
					width, 0, static_cast<GLenum>(format.pixelFormat.glLocalDataFormat),
					static_cast<GLenum>(format.pixelFormat.glLocalDataType), getLocalData());
			break;
		}
#endif
		case TextureType::TEXTURE_2D: {
			if(format.pixelFormat.compressed) {
				glCompressedTexImage2D(GL_TEXTURE_2D, level, static_cast<GLint>(format.pixelFormat.glInternalFormat),
										width, height, 0,
										static_cast<GLsizei>(format.compressedImageSize),
										getLocalData());
			}else{
					GET_GL_ERROR();
				glTexImage2D(GL_TEXTURE_2D, level, static_cast<GLint>(format.pixelFormat.glInternalFormat),
										width, height, 0,
										static_cast<GLenum>(format.pixelFormat.glLocalDataFormat), 
										static_cast<GLenum>(format.pixelFormat.glLocalDataType), getLocalData());
						GET_GL_ERROR();
			}
			break;
		}
		case TextureType::TEXTURE_CUBE_MAP:{
			Util::Reference<Util::PixelAccessor> pa =  Util::PixelAccessor::create(getLocalBitmap());
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
			}
			break;
		}
#if defined(LIB_GL)
		case  TextureType::TEXTURE_BUFFER:{
			if( getLocalBitmap() ){ // there is a local bitmap?
				bufferObject->uploadData(GL_TEXTURE_BUFFER,  getLocalBitmap()->data(),getLocalBitmap()->getDataSize(), GL_STATIC_DRAW );
			}else if( !bufferObject->isValid() ){ // allocate data for buffer
				bufferObject->uploadData(GL_TEXTURE_BUFFER,  nullptr,getDataSize(), GL_STATIC_DRAW );
			}else{
				// assume the buffer already contains the data; this may lead to a crash if this is not the case!
				// \todo if this can be checked somehow, do it!
			}			
			// special case:the used internalFormat in TextureUtils is not applicable here
			const auto& pixelFormat = getFormat().pixelFormat;
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
			}
			// bind the buffer to the texture
			glTexBuffer( GL_TEXTURE_BUFFER, format, bufferObject->getGLId() );
			break;
		
		}
		case  TextureType::TEXTURE_1D_ARRAY:{
			glTexImage2D(GL_TEXTURE_1D_ARRAY, level, static_cast<GLint>(format.pixelFormat.glInternalFormat),
							width, depth, 0,
							static_cast<GLenum>(format.pixelFormat.glLocalDataFormat), 
							static_cast<GLenum>(format.pixelFormat.glLocalDataType), getLocalData());
			break;
		}
		case  TextureType::TEXTURE_2D_ARRAY:
		case  TextureType::TEXTURE_3D:
		case  TextureType::TEXTURE_CUBE_MAP_ARRAY:{
			glTexImage3D(static_cast<GLenum>(format.glTextureType), level, static_cast<GLint>(format.pixelFormat.glInternalFormat),
							width, height, depth, 0,
							static_cast<GLenum>(format.pixelFormat.glLocalDataFormat), 
							static_cast<GLenum>(format.pixelFormat.glLocalDataType), getLocalData());
			break;
		}
		case TextureType::TEXTURE_2D_MULTISAMPLE: {
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, format.numSamples, static_cast<GLint>(format.pixelFormat.glInternalFormat),
									width, height, false);
			GET_GL_ERROR();
			break;
		}
#endif
		default:{
			context.popTexture(0);
			glActiveTexture(activeTexture);
			throw std::runtime_error("Texture::_uploadGLTexture: Unsupported texture type.");
		}
	}
	GET_GL_ERROR();
	context.popTexture(0);
	glActiveTexture(activeTexture);
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

void Texture::clearGLData(const Util::Color4f& color) {
	static const bool clearSupported = isExtensionSupported("GL_VERSION_4_4");
	if(!clearSupported){
		WARN("clearGLData: Requires at least OpenGL 4.4 to work.");
		return;
	}
#if defined(LIB_GL)
	if(!isGLTextureValid())
		return;
	
	int maxLevel = static_cast<int>(hasMipmaps ? std::log2(std::max(getWidth(), getHeight())) : 0);
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
#endif
}

void Texture::downloadGLTexture(RenderingContext & context) {
#ifdef LIB_GL
	if(!glId){
		WARN("downloadGLTexture: No glTexture available.");
		return;
	}
	dataHasChanged = false;

	if(!localBitmap)
		allocateLocalData();

	context.pushAndSetTexture(0,this);
	context.applyChanges();
	switch( tType ){
		case  TextureType::TEXTURE_1D:
		case  TextureType::TEXTURE_2D:
			glGetTexImage(format.glTextureType, 0, format.pixelFormat.glLocalDataFormat, format.pixelFormat.glLocalDataType, getLocalData());
			break;
		case  TextureType::TEXTURE_CUBE_MAP:{
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
		}
		case TextureType::TEXTURE_BUFFER:{
			auto data =  bufferObject->downloadData<uint8_t>(GL_TEXTURE_BUFFER,localBitmap->getDataSize());
			localBitmap->swapData( data );
			break;
		}
		case  TextureType::TEXTURE_1D_ARRAY:
		case  TextureType::TEXTURE_2D_ARRAY:
		case  TextureType::TEXTURE_3D:
			glGetTexImage(format.glTextureType, 0, format.pixelFormat.glLocalDataFormat, format.pixelFormat.glLocalDataType, getLocalData());
			break;
		case  TextureType::TEXTURE_CUBE_MAP_ARRAY:		
		default:
			throw std::runtime_error("Texture::downloadGLTexture: (currently) unsupported texture type.");
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
	} else {
		// remove local data
		localBitmap = nullptr;
		dataHasChanged = false;
	}
}


void Texture::enableComparision(RenderingContext & context, Comparison::function_t func) {
	if(!glId || dataHasChanged)
		_uploadGLTexture(context);
#ifdef LIB_GL
	context.pushAndSetTexture(0,this);
	context.applyChanges();
	glTexParameteri(format.glTextureType, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
	glTexParameteri(format.glTextureType, GL_TEXTURE_COMPARE_FUNC, Comparison::functionToGL(func));
	context.popTexture(0);
#endif
}

}

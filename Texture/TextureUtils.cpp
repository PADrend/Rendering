/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2014 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "TextureUtils.h"
#include "../Mesh/Mesh.h"
#include "../Mesh/MeshDataStrategy.h"
#include "../Mesh/MeshIndexData.h"
#include "../Mesh/MeshVertexData.h"
#include "../Mesh/VertexAttribute.h"
#include "../Mesh/VertexAttributeIds.h"
#include "../Mesh/VertexDescription.h"
#include "../RenderingContext/RenderingParameters.h"
#include "../RenderingContext/RenderingContext.h"
#include "../GLHeader.h"
#include "../Helper.h"
#include <Geometry/Definitions.h>
#include <Geometry/Matrix4x4.h>
#include <Geometry/Rect.h>
#include <Geometry/Vec2.h>
#include <Util/IO/FileName.h>
#include <Util/IO/FileUtils.h>
#include <Util/Graphics/Bitmap.h>
#include <Util/Graphics/PixelFormat.h>
#include <Util/Graphics/Color.h>
#include <Util/Graphics/NoiseGenerator.h>
#include <Util/Graphics/PixelAccessor.h>
#include <Util/Macros.h>
#include <Util/References.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace Rendering {
namespace TextureUtils {

//! (static)
PixelFormatGL pixelFormatToGLPixelFormat(const Util::PixelFormat & pixelFormat){
	PixelFormatGL glf;
	if(pixelFormat.getValueType() == Util::TypeConstant::UINT8){
		glf.glLocalDataType = GL_UNSIGNED_BYTE;
#if defined(LIB_GL)
		if( pixelFormat == Util::PixelFormat::MONO ){
			glf.glLocalDataFormat = GL_RED;
			glf.glInternalFormat = GL_RED; 
		}else if( pixelFormat == Util::PixelFormat(Util::TypeConstant::UINT8, 0, 1, Util::PixelFormat::NONE, Util::PixelFormat::NONE) ){
			glf.glLocalDataFormat = GL_RG;
			glf.glInternalFormat = GL_RG; 
		}else
#endif
		if( pixelFormat == Util::PixelFormat::RGB ){
			glf.glLocalDataFormat = GL_RGB;
			glf.glInternalFormat = GL_RGB; // GL_RGB8????
		}
#if defined(LIB_GL)
		else if( pixelFormat == Util::PixelFormat::BGR ){
			glf.glLocalDataFormat = GL_BGR;
			glf.glInternalFormat = GL_BGR; 
		} else if( pixelFormat == Util::PixelFormat::RGBA ){
			glf.glLocalDataFormat = GL_RGBA;
			glf.glInternalFormat = GL_RGBA; 
		}
#endif
	}
#if defined(LIB_GL)
	else if(pixelFormat.getValueType() == Util::TypeConstant::UINT32){
		glf.glLocalDataType = GL_UNSIGNED_INT;
		if( pixelFormat == Util::PixelFormat(Util::TypeConstant::UINT32, 0, Util::PixelFormat::NONE, Util::PixelFormat::NONE, Util::PixelFormat::NONE) ){
			glf.glLocalDataFormat = GL_RED_INTEGER;
			glf.glInternalFormat = GL_R32UI; 
		}else if( pixelFormat == Util::PixelFormat(Util::TypeConstant::UINT32, 0, 4, Util::PixelFormat::NONE, Util::PixelFormat::NONE) ){
			glf.glLocalDataFormat = GL_RG_INTEGER;
			glf.glInternalFormat = GL_RG32UI; 
		} else if( pixelFormat == Util::PixelFormat(Util::TypeConstant::UINT32, 0, 4, 8, Util::PixelFormat::NONE) ){
			glf.glLocalDataFormat = GL_RGB_INTEGER;
			glf.glInternalFormat = GL_RGB32UI;
		} else if( pixelFormat == Util::PixelFormat(Util::TypeConstant::UINT32, 0, 4, 8, 12) ){
			glf.glLocalDataFormat = GL_RGBA_INTEGER;
			glf.glInternalFormat = GL_RGBA32UI; 
		}
	} else if(pixelFormat.getValueType() == Util::TypeConstant::INT32){
		glf.glLocalDataType = GL_INT;
		if( pixelFormat == Util::PixelFormat(Util::TypeConstant::INT32, 0, Util::PixelFormat::NONE, Util::PixelFormat::NONE, Util::PixelFormat::NONE) ){
			glf.glLocalDataFormat = GL_RED_INTEGER;
			glf.glInternalFormat = GL_R32I; 
		}else if( pixelFormat == Util::PixelFormat(Util::TypeConstant::INT32, 0, 4, Util::PixelFormat::NONE, Util::PixelFormat::NONE) ){
			glf.glLocalDataFormat = GL_RG_INTEGER;
			glf.glInternalFormat = GL_RG32I; 
		} else if( pixelFormat == Util::PixelFormat(Util::TypeConstant::INT32, 0, 4, 8, Util::PixelFormat::NONE) ){
			glf.glLocalDataFormat = GL_RGB_INTEGER;
			glf.glInternalFormat = GL_RGB32I;
		} else if( pixelFormat == Util::PixelFormat(Util::TypeConstant::INT32, 0, 4, 8, 12) ){
			glf.glLocalDataFormat = GL_RGBA_INTEGER;
			glf.glInternalFormat = GL_RGBA32I; 
		}
	} else if(pixelFormat.getValueType() == Util::TypeConstant::FLOAT){
		glf.glLocalDataType = GL_FLOAT;
		if( pixelFormat == Util::PixelFormat::MONO_FLOAT ){
			glf.glLocalDataFormat = GL_RED;
			glf.glInternalFormat = GL_R32F; 
		}else if( pixelFormat == Util::PixelFormat(Util::TypeConstant::FLOAT, 0, 4, Util::PixelFormat::NONE, Util::PixelFormat::NONE) ){
			glf.glLocalDataFormat = GL_RG;
			glf.glInternalFormat = GL_RG32F; 
		} else if( pixelFormat == Util::PixelFormat::RGB_FLOAT ){
			glf.glLocalDataFormat = GL_RGB;
			glf.glInternalFormat = GL_RGB32F; // GL_RGB8????
		} else if( pixelFormat == Util::PixelFormat::RGBA_FLOAT ){
			glf.glLocalDataFormat = GL_RGBA;
			glf.glInternalFormat = GL_RGBA32F; 
		}
	}
#endif
	return glf;
}

//! (static)
Util::PixelFormat glPixelFormatToPixelFormat(const PixelFormatGL& glPixelFormat){
	using Util::PixelFormat;
	
	PixelFormat bitmapPixelFormat = PixelFormat::UNKNOWN;
	if(glPixelFormat.compressed)
		return bitmapPixelFormat;
		
#ifdef LIB_GL
	if(glPixelFormat.glLocalDataType==GL_FLOAT) {
		switch(glPixelFormat.glLocalDataFormat){
			case GL_RGBA:
				bitmapPixelFormat = PixelFormat::RGBA_FLOAT;
				break;
			case GL_RGB:
				bitmapPixelFormat = PixelFormat::RGB_FLOAT;
				break;
			case GL_BGRA:
				bitmapPixelFormat = PixelFormat::BGRA_FLOAT;
				break;
			case GL_BGR:
				bitmapPixelFormat = PixelFormat::BGR_FLOAT;
				break;
			case GL_DEPTH_COMPONENT:
			case GL_RED:
				bitmapPixelFormat = PixelFormat( Util::TypeConstant::FLOAT, 0,PixelFormat::NONE,PixelFormat::NONE,PixelFormat::NONE);
				break;
			case GL_GREEN:
				bitmapPixelFormat = PixelFormat( Util::TypeConstant::FLOAT, PixelFormat::NONE,0,PixelFormat::NONE,PixelFormat::NONE);
				break;
			case GL_BLUE:
				bitmapPixelFormat = PixelFormat( Util::TypeConstant::FLOAT, PixelFormat::NONE,PixelFormat::NONE,0,PixelFormat::NONE);
				break;
			case GL_ALPHA:
				bitmapPixelFormat = PixelFormat( Util::TypeConstant::FLOAT, PixelFormat::NONE,PixelFormat::NONE,PixelFormat::NONE,0);
				break;
			default:
//				WARN("Texture::allocateLocalData: Unsupported glFormat.");
				break;
		}
	}else if(glPixelFormat.glLocalDataType==GL_UNSIGNED_BYTE) {
		switch (glPixelFormat.glLocalDataFormat){
			case GL_RGBA:
				bitmapPixelFormat = PixelFormat::RGBA;
				break;
			case GL_RGB:
				bitmapPixelFormat = PixelFormat::RGB;
				break;
			case GL_BGRA:
				bitmapPixelFormat = PixelFormat::BGRA;
				break;
			case GL_BGR:
				bitmapPixelFormat = PixelFormat::BGR;
				break;
			case GL_DEPTH_COMPONENT:
			case GL_RED:
				bitmapPixelFormat = PixelFormat( Util::TypeConstant::UINT8, 0,PixelFormat::NONE,PixelFormat::NONE,PixelFormat::NONE);
				break;
			case GL_GREEN:
				bitmapPixelFormat = PixelFormat( Util::TypeConstant::UINT8, PixelFormat::NONE,0,PixelFormat::NONE,PixelFormat::NONE);
				break;
			case GL_BLUE:
				bitmapPixelFormat = PixelFormat( Util::TypeConstant::UINT8, PixelFormat::NONE,PixelFormat::NONE,0,PixelFormat::NONE);
				break;
			case GL_ALPHA:
				bitmapPixelFormat = PixelFormat( Util::TypeConstant::UINT8, PixelFormat::NONE,PixelFormat::NONE,PixelFormat::NONE,0);
				break;
			default:
//				WARN("Texture::allocateLocalData: Unsupported glFormat.");
				break;
		}
	}else if(glPixelFormat.glLocalDataType==GL_UNSIGNED_INT) {
		switch (glPixelFormat.glLocalDataFormat){
			case GL_RED_INTEGER:
				bitmapPixelFormat = PixelFormat( Util::TypeConstant::UINT32, 0, PixelFormat::NONE, PixelFormat::NONE, PixelFormat::NONE );
				break;
			case GL_RG_INTEGER:
				bitmapPixelFormat = PixelFormat( Util::TypeConstant::UINT32, 0, 4, PixelFormat::NONE, PixelFormat::NONE );
				break;
			case GL_RGB_INTEGER:
				bitmapPixelFormat = PixelFormat( Util::TypeConstant::UINT32, 0, 4, 8, PixelFormat::NONE );
				break;
			case GL_RGBA_INTEGER:
				bitmapPixelFormat = PixelFormat( Util::TypeConstant::UINT32, 0, 4, 8, 12 );
				break;
			default:
//				WARN("Texture::allocateLocalData: Unsupported glFormat.");
				break;
		}
	}else if(glPixelFormat.glLocalDataType==GL_INT) {
		switch (glPixelFormat.glLocalDataFormat){
			case GL_RED_INTEGER:
				bitmapPixelFormat = PixelFormat( Util::TypeConstant::INT32, 0, PixelFormat::NONE, PixelFormat::NONE, PixelFormat::NONE );
				break;
			case GL_RG_INTEGER:
				bitmapPixelFormat = PixelFormat( Util::TypeConstant::INT32, 0, 4, PixelFormat::NONE, PixelFormat::NONE );
				break;
			case GL_RGB_INTEGER:
				bitmapPixelFormat = PixelFormat( Util::TypeConstant::INT32, 0, 4, 8, PixelFormat::NONE );
				break;
			case GL_RGBA_INTEGER:
				bitmapPixelFormat = PixelFormat( Util::TypeConstant::INT32, 0, 4, 8, 12 );
				break;
			default:
//				WARN("Texture::allocateLocalData: Unsupported glFormat.");
				break;
		}
	} else if(glPixelFormat.glLocalDataType == GL_UNSIGNED_INT_24_8_EXT) {
		bitmapPixelFormat = PixelFormat::RGBA;
	}else{
//		WARN("Texture::allocateLocalData: Unsupported glDataType.");
	}

#else /* LIB_GL */

	if(glPixelFormat.glLocalDataType == GL_FLOAT) {
		switch (glPixelFormat.glLocalDataFormat) {
			case GL_RGBA:
				bitmapPixelFormat = PixelFormat::RGBA_FLOAT;
				break;
			case GL_RGB:
				bitmapPixelFormat = PixelFormat::RGB_FLOAT;
				break;
			default:
				break;
		}
	} else if(glPixelFormat.glLocalDataType == GL_UNSIGNED_BYTE) {
		switch (glPixelFormat.glLocalDataFormat) {
			case GL_RGBA:
				bitmapPixelFormat = PixelFormat::RGBA;
				break;
			case GL_RGB:
				bitmapPixelFormat = PixelFormat::RGB;
				break;
			default:
				break;
		}
	}

#endif /* LIB_GL */
	return bitmapPixelFormat;
}

//! (static)
uint32_t textureTypeToGLTextureType(TextureType type){
	switch(type){
#if defined(LIB_GL)
		case TextureType::TEXTURE_1D:
			return static_cast<uint32_t>(GL_TEXTURE_1D);
		case TextureType::TEXTURE_1D_ARRAY:
			return static_cast<uint32_t>(GL_TEXTURE_1D_ARRAY);
#endif
		case TextureType::TEXTURE_2D:
			return static_cast<uint32_t>(GL_TEXTURE_2D);
#if defined(LIB_GL)
		case TextureType::TEXTURE_2D_ARRAY:
			return static_cast<uint32_t>(GL_TEXTURE_2D_ARRAY);
		case TextureType::TEXTURE_3D:
			return static_cast<uint32_t>(GL_TEXTURE_3D);
#endif
		case TextureType::TEXTURE_CUBE_MAP:
			return static_cast<uint32_t>(GL_TEXTURE_CUBE_MAP);
#if defined(LIB_GL)
		case TextureType::TEXTURE_CUBE_MAP_ARRAY:
			return static_cast<uint32_t>(GL_TEXTURE_CUBE_MAP_ARRAY);
		case TextureType::TEXTURE_BUFFER:
			return static_cast<uint32_t>(GL_TEXTURE_BUFFER);
#endif
		default:
			throw std::logic_error("createTextureFromBitmap: Invalid type.");
	}
}

// ----------------------------------------------------------------------------
// factory functions

static Texture * create( TextureType type,uint32_t sizeX,uint32_t sizeY,uint32_t numLayers,GLenum glPixelFormat,GLenum glPixelDataType,GLenum glInternalFormat, bool filtering,bool clampToEdge=false){
	Texture::Format format;
	format.glTextureType = textureTypeToGLTextureType(type);
	format.sizeX = sizeX;
	format.sizeY = sizeY;
	format.numLayers = numLayers;
	format.pixelFormat.glLocalDataFormat = glPixelFormat;
	format.pixelFormat.glLocalDataType = glPixelDataType;
	format.pixelFormat.glInternalFormat = glInternalFormat;
	
	format.linearMinFilter = filtering;
	format.linearMagFilter = filtering;
	
	if(clampToEdge) {
		format.glWrapS = GL_CLAMP_TO_EDGE;
		format.glWrapT = GL_CLAMP_TO_EDGE;
		format.glWrapR = GL_CLAMP_TO_EDGE;
	}
	
	return new Texture(format);
}

//! [static] Factory
Util::Reference<Texture> createColorTexture(TextureType type,uint32_t sizeX,uint32_t sizeY, uint32_t numLayers, Util::TypeConstant dataType, uint8_t numComponents,bool filtering,bool clampToEdge/*=false*/){
	if( numComponents<1||numComponents>4 )
		throw std::logic_error("createTexture: Invalid numComponents.");
	
	const auto bytes = getNumBytes(dataType);
	auto glPixelFormat = pixelFormatToGLPixelFormat( Util::PixelFormat(dataType,
																		0,
																		numComponents>1 ? bytes : Util::PixelFormat::NONE, 
																		numComponents>2 ? bytes*2 : Util::PixelFormat::NONE, 
																		numComponents>3 ? bytes*3 : Util::PixelFormat::NONE));

	return create( type, sizeX, sizeY, numLayers, glPixelFormat.glLocalDataFormat, glPixelFormat.glLocalDataType, glPixelFormat.glInternalFormat, filtering, clampToEdge);
}

//! (static)
Util::Reference<Texture> createStdCubeTexture(uint32_t width, bool alpha) {
	return createColorTexture(TextureType::TEXTURE_CUBE_MAP, width, width, 6, Util::TypeConstant::UINT8, alpha ? 4 : 3,  true);
}

//! (static)
Util::Reference<Texture> createStdTexture(uint32_t width, uint32_t height, bool alpha) {
	return createColorTexture(TextureType::TEXTURE_2D, width, height, 1, Util::TypeConstant::UINT8, alpha ? 4 : 3,  true);
}

//! (static)
Util::Reference<Texture> createHDRCubeTexture(uint32_t width, bool alpha) {
	return createColorTexture(TextureType::TEXTURE_CUBE_MAP, width, width, 6, Util::TypeConstant::FLOAT, alpha ? 4 : 3, true);
}

//! (static)
Util::Reference<Texture> createHDRTexture(uint32_t width, uint32_t height, bool alpha) {
	return createColorTexture(TextureType::TEXTURE_2D, width, height, 1, Util::TypeConstant::FLOAT, alpha ? 4 : 3, true);
}

//! (static)
Util::Reference<Texture> createRedTexture(uint32_t width, uint32_t height, bool useByte) {
	return createColorTexture(TextureType::TEXTURE_2D, width, height, 1, useByte ? Util::TypeConstant::UINT8 : Util::TypeConstant::FLOAT, 1,  true);
}

//! (static)
Util::Reference<Texture> createDepthStencilTexture(uint32_t width, uint32_t height) {
#if defined(LIB_GL)
	return create(TextureType::TEXTURE_2D, width, height, 1, GL_DEPTH_STENCIL_EXT, GL_UNSIGNED_INT_24_8_EXT, GL_DEPTH24_STENCIL8_EXT, false);
#else
	return nullptr;
#endif
}

//! [static] Factory
Util::Reference<Texture> createDepthTexture(uint32_t width, uint32_t height) {
#if defined(LIB_GL)
	return create(TextureType::TEXTURE_2D, width, height, 1, GL_DEPTH_COMPONENT, GL_FLOAT, GL_DEPTH_COMPONENT, false);
#else
	return nullptr;
#endif
}

//! [static] Factory
Util::Reference<Texture> createDataTexture(TextureType type,uint32_t sizeX,uint32_t sizeY, uint32_t numLayers, Util::TypeConstant dataType, uint8_t numComponents){
	return createColorTexture(type,sizeX,sizeY,numLayers,dataType,numComponents,false);
}
// ----------------------------

Util::Reference<Texture> createNoiseTexture(uint32_t width, uint32_t height, bool alpha,float scaling) {
#if defined(LIB_GL)
	Util::Reference<Texture> texture = create(TextureType::TEXTURE_2D,width,height,1, alpha ? GL_RGBA : GL_RGB, GL_FLOAT,alpha ? GL_RGBA32F_ARB : GL_RGB32F_ARB,true);

	texture->allocateLocalData();
	Util::Reference<Util::PixelAccessor> pixelAccessor = Util::PixelAccessor::create(texture->getLocalBitmap());
	Util::NoiseGenerator generator(17);
	for(uint_fast32_t i = 0; i < static_cast<uint_fast32_t>(texture->getWidth()); ++i) {
		for(uint_fast32_t j = 0; j < static_cast<uint_fast32_t>(texture->getHeight()); ++j) {
			const float x = (i + 0.5f) * scaling;
			const float y = (j + 0.5f) * scaling;
			pixelAccessor->writeColor(i, j, Util::Color4f(
					(generator.get(x, y, 0.5f) + 1.0f) / 2.0f,
					(generator.get(x, y, 1.5f) + 1.0f) / 2.0f,
					(generator.get(x, y, 2.5f) + 1.0f) / 2.0f,
					(generator.get(x, y, 3.5f) + 1.0f) / 2.0f
			));
		}
	}
	texture->dataChanged();

	return texture.detachAndDecrease();
#else
	return nullptr;
#endif
}


Util::Reference<Texture> createTextureDataArray_Vec4(const uint32_t size) {
	return createDataTexture(TextureType::TEXTURE_1D,size,1,1,Util::TypeConstant::FLOAT,4); 
}

//! [static] Factory
Util::Reference<Texture> createChessTexture(uint32_t width, uint32_t height, int fieldSize_powOfTwo) {
	auto t = create(TextureType::TEXTURE_2D, width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA ,true);

	t->allocateLocalData();
	GLubyte * tData=t->getLocalData();

	GLubyte c;
	int k=0;
	for(uint_fast32_t i = 0; i < height; ++i) {
		for(uint_fast32_t j = 0; j < width; ++j) {
			c=(((i&fieldSize_powOfTwo)==0)^((j&fieldSize_powOfTwo)==0))*255;
			tData[k++]=c;
			tData[k++]=c;
			tData[k++]=c;
			tData[k++]=255;
		}
	}
	t->dataChanged();
	return t;
}

Util::Reference<Texture> createTextureFromBitmap(const Util::Bitmap & bitmap, TextureType type, uint32_t numLayers, bool clampToEdge){
	const uint32_t bHeight = bitmap.getHeight();
	const uint32_t width = bitmap.getWidth();

	Texture::Format format;

	if( numLayers==0 || numLayers>bHeight || (bHeight%numLayers) != 0){
		WARN("createTextureFromBitmap: Bitmap height is not dividable into given number of layers.");
		return nullptr;
	}
	
	format.glTextureType = textureTypeToGLTextureType( type );
	
	format.sizeY = bHeight / numLayers;
	format.sizeX = width;
	format.numLayers = numLayers;

	format.pixelFormat = pixelFormatToGLPixelFormat(bitmap.getPixelFormat());
	if(!format.pixelFormat.isValid()){
		WARN("createTextureFromBitmap: Bitmap has unimplemented pixel format.");
		return nullptr;
	}
	
	if(clampToEdge) {
		format.glWrapS = GL_CLAMP_TO_EDGE;
		format.glWrapT = GL_CLAMP_TO_EDGE;
		format.glWrapR = GL_CLAMP_TO_EDGE;
	}

	Util::Reference<Texture> texture = new Texture(format);
	texture->allocateLocalData();
	const uint8_t * pixels = bitmap.data();

	// Flip the rows.
	const uint32_t rowSize = width * bitmap.getPixelFormat().getBytesPerPixel();
	for (uint_fast16_t row = 0; row < bHeight; ++row) {
		const uint32_t offset = row * rowSize;
		const uint16_t reverseRow = bHeight - 1 - row;
		const uint32_t reverseOffset = reverseRow * rowSize;
		std::copy(pixels + reverseOffset, pixels + reverseOffset + rowSize, texture->getLocalData() + offset);
	}

	texture->dataChanged();
	return texture;
}

/**
 * [static]  Factory: Creates a Texture from a .raw file. Returns 0 on failure.
 * @Note: Used for importing hight-maps e.g. created with terragen.
 * @todo Create a Streamer class instead of this function.
 */
Util::Reference<Texture> createTextureFromRAW(const Util::FileName & filename, unsigned int type,  bool flip_h) {
	if(type!=RAW_16BIT_BW) {
		WARN(std::string("RAW-Image has unimplemented color format for file ") + filename);
		return nullptr;
	}
	const std::vector<uint8_t> buffer = Util::FileUtils::loadFile(filename);
	if(buffer.empty()) {
		WARN(std::string("Could not open file ") + filename.toString());
		return nullptr;
	}
	uint32_t width = static_cast<uint32_t> (std::sqrt(buffer.size()/2.0));
//    std::cout <<"\n\nWidth:"<<width<<","<<size<<"\n\n";
	if(!width*width*2 == buffer.size()) {
		WARN(std::string("RAW-Image is not quadratic for file ") + filename.toString());
		return nullptr;
	}

	Texture::Format format;
	format.glTextureType = GL_TEXTURE_2D;
	format.sizeY = width;
	format.sizeX = width;
	format.pixelFormat.glLocalDataType = GL_FLOAT;
	format.pixelFormat.glInternalFormat = GL_RGB;
	format.pixelFormat.glLocalDataFormat = GL_RGB;

	// TODO! check endianess!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	auto t=new Texture(format);
	t->allocateLocalData();
	float * data = reinterpret_cast<float *> (t->getLocalData());//new float[width*width*3];

	for (uint32_t line=0;line<width;++line) {
		uint32_t v=flip_h ? width*(width-line-1) : width*line;
		for (uint32_t i=0;i<width;++i) {
			float f = static_cast<float> (( reinterpret_cast<const unsigned short *> (buffer.data())[v+i])/ static_cast<float> (0xFFFF));
		//        std::cout << f << " ";
			uint32_t p=(width*line+i)*3;
			data[p+0]=f;
			data[p+1]=f;
			data[p+2]=f;
		}
	}
	t->dataChanged();
	return t;
}

//! [static]
Util::Reference<Texture> createTextureFromScreen(int xpos, int ypos, const Texture::Format & format) {
	auto texture = new Texture(format);
	texture->allocateLocalData();
	glReadPixels(xpos, ypos, static_cast<GLsizei>(format.sizeX), static_cast<GLsizei>(format.sizeY), 
				format.pixelFormat.glLocalDataFormat, format.pixelFormat.glLocalDataType, texture->getLocalData());
	return texture;
}

//! [static]
Util::Reference<Texture> createTextureFromScreen(int xpos/*=0*/, int ypos/*=0*/, int width/*=-1*/, int height/*=-1*/,bool useAlpha){
	 if(width < 0 || height <0){
		GLint viewport[4];
		glGetIntegerv(GL_VIEWPORT, viewport);
		if(width<0)
			width = viewport[2]-xpos;
		if(height<0)
			height = viewport[3]-ypos;
	 }
	Texture::Format format;
	format.sizeX = width;
	format.sizeY = height;
	format.pixelFormat.glLocalDataType = GL_UNSIGNED_BYTE;
	format.pixelFormat.glLocalDataFormat = useAlpha ? GL_RGBA : GL_RGB;
	format.pixelFormat.glInternalFormat = useAlpha ? GL_RGBA : GL_RGB;
	return createTextureFromScreen(xpos, ypos, format);
}

// ---------------------------------------------------------------------------------------------------------------------------------------------


//! [static]
bool compareTextures(Texture *t1, Texture *t2) {
	if(t1 == t2)
		return true;
	if(t1 == nullptr || t2 == nullptr
			|| t1->getLocalData() == nullptr
			|| t2->getLocalData() == nullptr)
		return false;

	// so, now we got to check the data
	const Texture::Format& f1 = t1->getFormat();
	const Texture::Format& f2 = t2->getFormat();

	if(f1.getDataSize() != f2.getDataSize()) {
		return false;
	}

	return std::equal(t1->getLocalData(), t1->getLocalData() + f1.getDataSize(), t2->getLocalData());
}

//! [static]
void updateTextureFromScreen(RenderingContext & context,Texture & t,const Geometry::Rect_i & textureRect, int screenPosX/*=0*/, int screenPosY/*=0*/){
	const Texture::Format & format = t.getFormat();
	const int width=textureRect.getWidth()>static_cast<int>(format.sizeX) ? static_cast<int>(format.sizeX) : textureRect.getWidth();
	const int height=textureRect.getHeight()>static_cast<int>(format.sizeY) ? static_cast<int>(format.sizeY) : textureRect.getHeight();
	context.pushAndSetTexture(0,&t);
	glCopyTexSubImage2D(GL_TEXTURE_2D,0,textureRect.getX(), textureRect.getY(),screenPosX,screenPosY, width, height);
	context.popTexture(0);
	GET_GL_ERROR();
}

//! [static]
void updateTextureFromScreen(RenderingContext & context,Texture & t){
	updateTextureFromScreen(context,t,Geometry::Rect_i(0,0,t.getFormat().sizeX,t.getFormat().sizeY));
}
//! [static]
void drawTextureToScreen(RenderingContext&rc,const Geometry::Rect_i & screenRect,Texture & t,const Geometry::Rect_f & textureRect){
#ifdef LIB_GL
	std::vector<Texture *> textures;
	textures.push_back(&t);
	std::vector<Geometry::Rect_f> rects;
	rects.push_back(textureRect);

	drawTextureToScreen(rc,screenRect,textures,rects);
#endif
}

//! (static)
void drawTextureToScreen(RenderingContext & rc, const Geometry::Rect_i & screenRect, const std::vector<Texture *> & textures,
		const std::vector<Geometry::Rect_f> & textureRects) {
#ifdef LIB_GL
	uint8_t numTextures = textures.size() < textureRects.size() ? textures.size() : textureRects.size();
	if(numTextures == 0) {
		return;
	}
	if(numTextures > 8) {
		WARN("At most eight textures are supported.");
		numTextures = 8;
	}

	rc.pushAndSetDepthBuffer(DepthBufferParameters(false, false, Comparison::LESS));
	rc.pushAndSetLighting(LightingParameters(false));
	rc.applyChanges();

	{
		const Geometry::Rect_i & viewport = rc.getViewport();

		rc.pushMatrix_cameraToClipping();
		rc.setMatrix_cameraToClipping(Geometry::Matrix4x4::orthographicProjection(0, viewport.getWidth(), 0, viewport.getHeight(), -1, 1));
	}
	{
		Geometry::Matrix4x4 identityMatrix;
		identityMatrix.setIdentity();

		rc.pushMatrix_modelToCamera();
		rc.setMatrix_modelToCamera(identityMatrix);
	}

	for(uint_fast8_t i = 0; i < numTextures; ++i) {
		rc.pushTexture(i);
		if(textures[i] == nullptr) {
			WARN("No Texture!");
			continue;
		}
		rc.setTexture(i, textures[i]);
	}

	// create mesh
	VertexDescription vertexDesc;
	const VertexAttribute & posAttr = vertexDesc.appendPosition2D();
	const VertexAttribute & colorAttr = vertexDesc.appendColorRGBAByte();
	std::vector<VertexAttribute> texCoordAttr;
	texCoordAttr.reserve(numTextures);
	for(uint_fast8_t i = 0; i < numTextures; ++i) {
		texCoordAttr.push_back(vertexDesc.appendAttribute(VertexAttributeIds::getTextureCoordinateIdentifier(i), 2, GL_FLOAT,false));
	}

	Util::Reference<Mesh> mesh = new Mesh(vertexDesc, 4, 6);
	mesh->setDataStrategy(SimpleMeshDataStrategy::getPureLocalStrategy());

	// init vertex data
	uint8_t * vertex = mesh->openVertexData().data();
	for(uint_fast8_t cornerNr=0;cornerNr<4;++cornerNr){
		const Geometry::rectCorner_t corner(static_cast<const Geometry::rectCorner_t>(cornerNr));

		// position
		const Geometry::Vec2 pos(screenRect.getCorner(corner));
		float * positionPtr = reinterpret_cast<float *>(vertex + posAttr.getOffset());
		positionPtr[0] = pos.getX();
		positionPtr[1] = pos.getY();

		// color
		uint8_t * color = reinterpret_cast<uint8_t *>(vertex + colorAttr.getOffset());
		std::fill_n(color, 4, 255);

		// texture coordinates
		for(uint_fast8_t i = 0; i < numTextures; ++i) {
			float * texCoordPtr = reinterpret_cast<float *>(vertex + texCoordAttr[i].getOffset());
			const Geometry::Vec2 uv(textureRects[i].getCorner(corner));
			texCoordPtr[0] = uv.getX();
			texCoordPtr[1] = uv.getY();
		}
		vertex += vertexDesc.getVertexSize();
	}

	{	// init index data
		static const uint32_t indices[] = { 0,1,2,1,3,2 };

		MeshIndexData & indexData = mesh->openIndexData();
		std::copy(indices,indices+6,indexData.data());
		indexData.updateIndexRange();
	}

	rc.displayMesh(mesh.get());

	for(uint_fast8_t i = 0; i < numTextures; ++i) {
		rc.popTexture(i);
	}

	rc.popMatrix_cameraToClipping();
	rc.popMatrix_modelToCamera();

	rc.popLighting();

	rc.popDepthBuffer();
#endif
}

Util::Reference<Util::Bitmap> createBitmapFromTexture(RenderingContext & context,Texture & texture) {
	if(texture.getLocalData() == nullptr){
		if(!texture.isGLTextureValid()){
			WARN("Error creating bitmap: texture has no local data and gl data invalid");
			return nullptr;
		}
		texture.downloadGLTexture(context);
	}
	return std::move(createBitmapFromLocalTexture(texture));
}

Util::Reference<Util::Bitmap> createBitmapFromLocalTexture(const Texture & texture) {
	const Util::Bitmap* sourceBitmap = texture.getLocalBitmap();
	if( !sourceBitmap ) {
		WARN("Texture has no local data; can not create Bitmap.");
		return nullptr;
	}
	Util::Reference<Util::Bitmap> targetBitmap = new Util::Bitmap(*sourceBitmap);
	targetBitmap->flipVertically();
	return targetBitmap;
}

Util::Reference<Util::PixelAccessor> createColorPixelAccessor(RenderingContext & context, Texture& texture) {
	texture.openLocalData(context);
	return Util::PixelAccessor::create(texture.getLocalBitmap());
}

Util::Reference<Util::PixelAccessor> createDepthPixelAccessor(RenderingContext & context, Texture& texture) {
	class DepthAccessor : public Util::PixelAccessor {
		public:
			DepthAccessor(Util::Reference<Util::Bitmap> bitmap) :
				Util::PixelAccessor(std::move(bitmap)) {
			}
			virtual ~DepthAccessor(){
			}

		private:
			//! Return depth value in red channel
			Util::Color4f doReadColor4f(uint32_t /*x*/ ,uint32_t /*y*/) const override {
				throw std::logic_error("Unsupported function called");
			}
			//! Return depth value in red channel
			Util::Color4ub doReadColor4ub(uint32_t /*x*/, uint32_t /*y*/) const override {
				throw std::logic_error("Unsupported function called");
			}

			//! ---|> PixelAccessor
			float doReadSingleValueFloat(uint32_t x, uint32_t y) const override {
				const uint32_t * const p = _ptr<uint32_t>(x, y);
				const uint32_t depthInt = ((*p) & 0xFFFFFF00) >> 8;
				return depthInt / static_cast<float>(0x00FFFFFF - 1); // (2^24 - 1)
			}

			//! ---|> PixelAccessor
			uint8_t doReadSingleValueByte(uint32_t x, uint32_t y) const override {
				const uint32_t * const p = _ptr<uint32_t>(x, y);
				const uint32_t depthInt = ((*p) & 0xFFFFFF00) >> 8;
				return depthInt / 65793; // depthInt / (2^24 - 1) * (2^8 - 1)
			}

			void doWriteColor(uint32_t /*x*/, uint32_t /*y*/, const Util::Color4f & /*c*/) override {
				throw std::logic_error("Unsupported function called");
			}

			void doWriteColor(uint32_t /*x*/, uint32_t /*y*/, const Util::Color4ub & /*c*/) override {
				throw std::logic_error("Unsupported function called");
			}

			void doWriteSingleValueFloat(uint32_t /*x*/, uint32_t /*y*/, float /*value*/) override {
				throw std::logic_error("Unsupported function called");
			}

	};
#ifdef LIB_GL
	if(texture.getFormat().pixelFormat.glLocalDataFormat != GL_DEPTH_STENCIL_EXT) {
#endif /* LIB_GL */
		return createColorPixelAccessor(context, texture);
#ifdef LIB_GL
	}
	texture.openLocalData(context);
	return new DepthAccessor(texture.getLocalBitmap());
#endif /* LIB_GL */
}

Util::Reference<Util::PixelAccessor> createStencilPixelAccessor(RenderingContext & context, Texture& texture) {
	class StencilAccessor : public Util::PixelAccessor {
		public:
			StencilAccessor(Util::Reference<Util::Bitmap> bitmap) :
				Util::PixelAccessor(std::move(bitmap)) {
			}
			virtual ~StencilAccessor(){
			}

		private:
			//! Return stencil value in red channel
			Util::Color4f doReadColor4f(uint32_t /*x*/, uint32_t /*y*/) const override {
				throw std::logic_error("Unsupported function called");
			}
			//! Return stencil value in red channel
			Util::Color4ub doReadColor4ub(uint32_t /*x*/, uint32_t /*y*/) const override {
				throw std::logic_error("Unsupported function called");
			}

			//! ---|> PixelAccessor
			float doReadSingleValueFloat(uint32_t x, uint32_t y) const override {
				const uint8_t stencilByte = *_ptr<uint8_t>(x, y);
				return stencilByte / static_cast<float>(0xFF - 1); // (2^8 - 1)
			}

			//! ---|> PixelAccessor
			uint8_t doReadSingleValueByte(uint32_t x, uint32_t y) const override {
				return *_ptr<uint8_t>(x, y);
			}

			void doWriteColor(uint32_t /*x*/, uint32_t /*y*/, const Util::Color4f & /*c*/) override {
				throw std::logic_error("Unsupported function called");
			}

			void doWriteColor(uint32_t /*x*/, uint32_t /*y*/, const Util::Color4ub & /*c*/) override {
				throw std::logic_error("Unsupported function called");
			}

			void doWriteSingleValueFloat(uint32_t /*x*/, uint32_t /*y*/, float /*value*/) override {
				throw std::logic_error("Unsupported function called");
			}

	};
#ifdef LIB_GL
	if(texture.getFormat().pixelFormat.glLocalDataFormat != GL_DEPTH_STENCIL_EXT) {
#endif /* LIB_GL */
		return createColorPixelAccessor(context, texture);
#ifdef LIB_GL
	}
	texture.openLocalData(context);
	return new StencilAccessor(texture.getLocalBitmap());
#endif /* LIB_GL */
}

float minDepthDistance(RenderingContext & context, Texture& firstTex, Texture& secondTex) {
	// check parameter validity
	const uint32_t width = static_cast<const uint32_t>(firstTex.getWidth());
	const uint32_t height = static_cast<const uint32_t>(firstTex.getHeight());
	if(width == 0 || height == 0) {
		INVALID_ARGUMENT_EXCEPTION("Textures may not have a size of 0.");
	}
	if(width != static_cast<const uint32_t>(secondTex.getWidth()) || height != static_cast<const uint32_t>(secondTex.getHeight())) {
		INVALID_ARGUMENT_EXCEPTION("Texture second has to be of the same size as firstTex.");
	}

	// download and open textures
	firstTex.downloadGLTexture(context);
	const float * firstData = reinterpret_cast<const float *>(firstTex.openLocalData(context));
	secondTex.downloadGLTexture(context);
	const float * secondData = reinterpret_cast<const float *>(secondTex.openLocalData(context));

	// main comparison
	// the textures are disjoint, if they don't have a common pixel with a depth value unequal to the clearDepth-value
	// (1.0f for firstTex and 0.0f for secondTex, since is inverted)
	bool disjoint = true;
	// minDifference; initialized with 1.0f since the depth values are clamped to [0, 1]
	float minDifference = 1.0f;
	for(uint32_t x = 0; x < width; ++x) {
		for(uint32_t y = 0; y < height; ++y) {
			const float first = firstData[y * width + x];
			// secondTex is flipped horizontally and inverted
			const float second = 1.0f - secondData[y * width + (width - x - 1)];
			// check whether the textures are disjoint
			if(first != 1.0f && second != 0.0f) {
				disjoint = false;
			}
			// determine the difference and update the minDifference
			const float difference = first - second;
			if(difference < minDifference) {
				minDifference = difference;
			}
		}
	}

	// check for errors and return according value (see the method documentation in the header file)
	if(minDifference < 0.0f) {
		return -1.0f;
	} else if(disjoint) {
		return -2.0f;
	} else {
		return minDifference;
	}
}

}
}

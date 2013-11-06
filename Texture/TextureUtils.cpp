/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
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
#include "../RenderingContext/ParameterStructs.h"
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

/*! (static) Factory */
Texture * TextureUtils::createStdTexture(uint32_t width, uint32_t height, bool alpha, bool useMipmaps, bool clampToEdge) {
	Texture::Format f;
	f.glTextureType=GL_TEXTURE_2D;
	f.width=width;
	f.height=height;
	f.glFormat=alpha ? GL_RGBA : GL_RGB;
	f.glDataType=GL_UNSIGNED_BYTE;
	f.glInternalFormat=alpha ? GL_RGBA : GL_RGB;
	f.magFilter=GL_NEAREST;
	f.minFilter=useMipmaps ? GL_NEAREST_MIPMAP_LINEAR : GL_LINEAR;

	if (clampToEdge) {
		f.wrapS = GL_CLAMP_TO_EDGE;
		f.wrapT = GL_CLAMP_TO_EDGE;
		f.wrapR = GL_CLAMP_TO_EDGE;
	}

	return new Texture(f);
}

Texture * TextureUtils::createNoiseTexture(uint32_t width, uint32_t height, bool alpha, bool useMipmaps, float scaling) {
	Texture::Format format;
	format.glTextureType = GL_TEXTURE_2D;
	format.width = width;
	format.height = height;
	format.glFormat = alpha ? GL_RGBA : GL_RGB;
	format.glDataType = GL_UNSIGNED_BYTE;
	format.glInternalFormat = alpha ? GL_RGBA : GL_RGB;
	format.magFilter = GL_LINEAR;
	format.minFilter = useMipmaps ? GL_NEAREST_MIPMAP_LINEAR : GL_LINEAR;

	Util::Reference<Texture> texture = new Texture(format);
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
}


Texture * TextureUtils::createTextureDataArray_Vec4(const uint32_t size) {
#if defined(LIB_GL)
	Texture::Format f;
	f.glTextureType = GL_TEXTURE_1D;
	f.width = size;
	f.height = 1;
	f.glFormat = GL_RGBA;
	f.glDataType = GL_FLOAT;
	f.glInternalFormat = GL_RGBA32F_ARB;
	f.magFilter = GL_LINEAR;
	f.minFilter = GL_LINEAR;
	f.wrapS = GL_CLAMP;
	f.wrapT = GL_CLAMP;
	return new Texture(f);
#else
	return nullptr;
#endif
}

#ifdef LIB_GL
/*! (static) Factory */
Texture * TextureUtils::createHDRTexture(uint32_t width, uint32_t height, bool alpha, bool useMipmaps) {
	Texture::Format f;
	f.glTextureType=GL_TEXTURE_2D;
	f.width=width;
	f.height=height;
	f.glFormat=alpha ? GL_RGBA : GL_RGB;
	f.glDataType=GL_FLOAT;
	f.glInternalFormat=alpha ? GL_RGBA32F_ARB : GL_RGB32F_ARB;
	f.magFilter=GL_LINEAR;
	f.minFilter=useMipmaps ? GL_NEAREST_MIPMAP_LINEAR : GL_LINEAR;

	return new Texture(f);
}
/*! (static) Factory */
Texture * TextureUtils::createRedTexture(uint32_t width, uint32_t height, bool useByte, bool useMipmaps) {
	Texture::Format f;
	f.glTextureType = GL_TEXTURE_2D;
	f.width = width;
	f.height = height;
	f.glFormat = GL_RED;
	f.glDataType = useByte ? GL_UNSIGNED_BYTE : GL_FLOAT;
	f.glInternalFormat = useByte ? 1 : GL_R32F;
	f.magFilter = GL_NEAREST;
	f.minFilter = useMipmaps ? GL_NEAREST_MIPMAP_LINEAR : GL_LINEAR;
	return new Texture(f);
}

/*! (static) Factory */
Texture * TextureUtils::createDepthStencilTexture(uint32_t width, uint32_t height) {
	Texture::Format depthStencilFormat;
	depthStencilFormat.glTextureType = GL_TEXTURE_2D;
	depthStencilFormat.width = width;
	depthStencilFormat.height = height;
	depthStencilFormat.glFormat = GL_DEPTH_STENCIL_EXT;
	depthStencilFormat.glDataType = GL_UNSIGNED_INT_24_8_EXT;
	depthStencilFormat.glInternalFormat = GL_DEPTH24_STENCIL8_EXT;
	depthStencilFormat.magFilter = GL_NEAREST;
	depthStencilFormat.minFilter = GL_NEAREST;

	return new Texture(depthStencilFormat);
}
#endif

/*! (static) Factory */
Texture * TextureUtils::createDepthTexture(uint32_t width, uint32_t height) {
	Texture::Format depthFormat;
	depthFormat.glTextureType = GL_TEXTURE_2D;
	depthFormat.width = width;
	depthFormat.height = height;
	depthFormat.glFormat = GL_DEPTH_COMPONENT;
	depthFormat.glDataType = GL_FLOAT;
	depthFormat.glInternalFormat = GL_DEPTH_COMPONENT;
	depthFormat.magFilter = GL_NEAREST;
	depthFormat.minFilter = GL_NEAREST;

	return new Texture(depthFormat);
}

//! [static] Factory
Texture * TextureUtils::createChessTexture(uint32_t width, uint32_t height, int fieldSize_powOfTwo, bool useMipmaps) {
	Texture::Format f=Texture::Format();
	f.glTextureType=GL_TEXTURE_2D;
	f.width = width;
	f.height = height;
	f.glFormat=GL_RGBA;
	f.glDataType=GL_UNSIGNED_BYTE;
	f.glInternalFormat=GL_RGBA;
	f.magFilter=GL_NEAREST;
	f.minFilter=useMipmaps ? GL_NEAREST_MIPMAP_LINEAR : GL_LINEAR;

	auto t=new Texture(f);
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

Texture * TextureUtils::createTextureFromBitmap(Util::Bitmap * bitmap, bool useMipmaps, bool clampToEdge) {
	const Util::PixelFormat & bFormat=bitmap->getPixelFormat();

	const uint32_t height = bitmap->getHeight();
	const uint32_t width = bitmap->getWidth();

	Texture::Format tFormat;
	tFormat.glTextureType = GL_TEXTURE_2D;
	tFormat.glDataType = GL_UNSIGNED_BYTE;
	tFormat.height = height;
	tFormat.width = width;

	if(bFormat==Util::PixelFormat::RGBA){
		tFormat.glFormat = GL_RGBA;
		tFormat.glInternalFormat = GL_RGBA;
	}else if(bFormat==Util::PixelFormat::RGB){
		tFormat.glFormat = GL_RGB;
		tFormat.glInternalFormat = GL_RGB;
#ifdef LIB_GL
	}else if(bFormat==Util::PixelFormat::BGRA){
		tFormat.glFormat = GL_BGRA;
		tFormat.glInternalFormat = GL_RGBA;
	}else if(bFormat==Util::PixelFormat::BGR){
		tFormat.glFormat = GL_BGR;
		tFormat.glInternalFormat = GL_RGB;
	}else if(bFormat==Util::PixelFormat::MONO){
		tFormat.glFormat = GL_RED;
		tFormat.glInternalFormat = GL_RED;
#endif /* LIB_GL */
	}else{
		WARN("createTextureFromBitmap: Bitmap has unimplemented color format.");
		return nullptr;
	}

	tFormat.minFilter = useMipmaps ? GL_NEAREST_MIPMAP_LINEAR : GL_LINEAR;
	if (clampToEdge) {
		tFormat.wrapS = GL_CLAMP_TO_EDGE;
		tFormat.wrapT = GL_CLAMP_TO_EDGE;
		tFormat.wrapR = GL_CLAMP_TO_EDGE;
	}

	auto texture = new Texture(tFormat);
	texture->allocateLocalData();
	const uint8_t * pixels = bitmap->data();

	// Flip the rows.
	const uint32_t rowSize = width * bFormat.getBytesPerPixel();
	for (uint_fast16_t row = 0; row < height; ++row) {
		const uint32_t offset = row * rowSize;
		const uint16_t reverseRow = height - 1 - row;
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
Texture * TextureUtils::createTextureFromRAW(const Util::FileName & filename, unsigned int type, bool useMipmaps, bool clampToEdge, bool flip_h) {
	if (type!=RAW_16BIT_BW) {
		WARN(std::string("RAW-Image has unimplemented color format for file ") + filename);
		return nullptr;
	}
	const std::vector<uint8_t> buffer = Util::FileUtils::loadFile(filename);
	if (buffer.empty()) {
		WARN(std::string("Could not open file ") + filename.toString());
		return nullptr;
	}
	uint32_t width = static_cast<uint32_t> (std::sqrt(buffer.size()/2.0));
//    std::cout <<"\n\nWidth:"<<width<<","<<size<<"\n\n";
	if (!width*width*2 == buffer.size()) {
		WARN(std::string("RAW-Image is not quadratic for file ") + filename.toString());
		return nullptr;
	}

	Texture::Format tFormat;
	tFormat.glTextureType=GL_TEXTURE_2D;
	tFormat.height = width;
	tFormat.width = width;
	tFormat.glDataType=GL_FLOAT;
	tFormat.glInternalFormat=GL_RGB;

	tFormat.glFormat = GL_RGB;
	tFormat.minFilter = useMipmaps ? GL_NEAREST_MIPMAP_LINEAR : GL_LINEAR;
	if (clampToEdge) {
		tFormat.wrapS=GL_CLAMP_TO_EDGE;
		tFormat.wrapT=GL_CLAMP_TO_EDGE;
		tFormat.wrapR=GL_CLAMP_TO_EDGE;
	}
	// TODO! check endianess!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	auto t=new Texture(tFormat);
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
bool TextureUtils::compareTextures(Texture *t1, Texture *t2) {
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
Texture * TextureUtils::createTextureFromScreen(int xpos, int ypos, const Texture::Format & format) {
	auto texture = new Texture(format);
	texture->allocateLocalData();
	glReadPixels(xpos, ypos, static_cast<GLsizei>(format.width), static_cast<GLsizei>(format.height), format.glFormat, format.glDataType, texture->getLocalData());
	return texture;
}

//! [static]
Texture * TextureUtils::createTextureFromScreen(int xpos/*=0*/, int ypos/*=0*/, int width/*=-1*/, int height/*=-1*/,bool useAlpha){
	 if(width < 0 || height <0){
		GLint viewport[4];
		glGetIntegerv(GL_VIEWPORT, viewport);
		if(width<0)
			width = viewport[2]-xpos;
		if(height<0)
			height = viewport[3]-ypos;
	 }
	Texture::Format format;
	format.width = width;
	format.height = height;
	format.glDataType = GL_UNSIGNED_BYTE;
	format.glFormat = useAlpha ? GL_RGBA : GL_RGB;
	return createTextureFromScreen(xpos, ypos, format);
}

//! [static]
void  TextureUtils::updateTextureFromScreen(RenderingContext & context,Texture * t,const Geometry::Rect_i & textureRect, int screenPosX/*=0*/, int screenPosY/*=0*/){
	const Texture::Format & f=t->getFormat();
	const int width=textureRect.getWidth()>static_cast<int>(f.width) ? static_cast<int>(f.width) : textureRect.getWidth();
	const int height=textureRect.getHeight()>static_cast<int>(f.height) ? static_cast<int>(f.height) : textureRect.getHeight();
	context.pushAndSetTexture(0,t);
	glCopyTexSubImage2D(GL_TEXTURE_2D,0,textureRect.getX(), textureRect.getY(),screenPosX,screenPosY, width, height);
	context.popTexture(0);
	GET_GL_ERROR();
}

//! [static]
void  TextureUtils::updateTextureFromScreen(RenderingContext & context,Texture * t){
	updateTextureFromScreen(context,t,Geometry::Rect_i(0,0,t->getFormat().width,t->getFormat().height));
}

#ifdef LIB_GL

//! [static]
void  TextureUtils::drawTextureToScreen(RenderingContext&rc,const Geometry::Rect_i & screenRect,Texture * t,const Geometry::Rect_f & textureRect){
	if(!t)
		return;
	std::vector<Texture *> textures;
	textures.push_back(t);
	std::vector<Geometry::Rect_f> rects;
	rects.push_back(textureRect);

	TextureUtils::drawTextureToScreen(rc,screenRect,textures,rects);
}

//! (static)
void TextureUtils::drawTextureToScreen(RenderingContext & rc, const Geometry::Rect_i & screenRect, const std::vector<Texture *> & textures,
		const std::vector<Geometry::Rect_f> & textureRects) {

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

		rc.pushProjectionMatrix();
		rc.setProjectionMatrix(Geometry::Matrix4x4::orthographicProjection(0, viewport.getWidth(), 0, viewport.getHeight(), -1, 1));
	}
	{
		Geometry::Matrix4x4 identityMatrix;
		identityMatrix.setIdentity();

		rc.pushMatrix();
		rc.setMatrix(identityMatrix);
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
		texCoordAttr.push_back(vertexDesc.appendAttribute(VertexAttributeIds::getTextureCoordinateIdentifier(i), 2, GL_FLOAT));
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

	rc.popProjectionMatrix();
	rc.popMatrix();

	rc.popLighting();

	rc.popDepthBuffer();
}
#endif

Util::Bitmap * TextureUtils::createBitmapFromTexture(RenderingContext & context,Texture * texture) {
	if (texture == nullptr){
		WARN("Error creating bitmap: texture was null");
		return nullptr;
	}
	if (texture->getLocalData() == nullptr){
		if(!texture->isGLTextureValid()){
			WARN("Error creating bitmap: texture has no local data and gl data invalid");
			return nullptr;
		}
		texture->downloadGLTexture(context);
	}
	return createBitmapFromLocalTexture(texture);
}

Util::Bitmap * TextureUtils::createBitmapFromLocalTexture(Texture * texture) {
	if (texture == nullptr) {
		return nullptr;
	}
	if (texture->getLocalData() == nullptr) {
		WARN("Texture has no local data; can not create Bitmap.");
		return nullptr;
	}

	const Texture::Format & tFormat = texture->getFormat();

	if (tFormat.glTextureType != GL_TEXTURE_2D) {
		WARN("createBitmapFromTexture: Other texture types than GL_TEXTURE_2D are not supported.");
		return nullptr;
	}

	Util::Bitmap * bitmap = nullptr;

	switch (tFormat.glFormat) {
#ifdef LIB_GL
		case GL_RED:
		case GL_GREEN:
		case GL_BLUE:
		case GL_ALPHA:
#endif /* LIB_GL */
		case GL_DEPTH_COMPONENT:{
			if(tFormat.glDataType == GL_UNSIGNED_BYTE){
				bitmap = new Util::Bitmap(static_cast<uint32_t>(tFormat.width), static_cast<uint32_t>(tFormat.height), Util::PixelFormat::MONO);
			}else if(tFormat.glDataType == GL_FLOAT){
				bitmap = new Util::Bitmap(static_cast<uint32_t>(tFormat.width), static_cast<uint32_t>(tFormat.height), Util::PixelFormat::MONO_FLOAT);
			}
			break;
		}
		case GL_RGB:{
			if(tFormat.glDataType == GL_UNSIGNED_BYTE)
				bitmap = new Util::Bitmap(static_cast<uint32_t>(tFormat.width), static_cast<uint32_t>(tFormat.height), Util::PixelFormat::RGB);
			break;
		}
		case GL_RGBA:{
			if(tFormat.glDataType == GL_UNSIGNED_BYTE)
				bitmap = new Util::Bitmap(static_cast<uint32_t>(tFormat.width), static_cast<uint32_t>(tFormat.height), Util::PixelFormat::RGBA);
			break;
		}
#ifdef LIB_GL
		case GL_BGR:{
			if(tFormat.glDataType == GL_UNSIGNED_BYTE)
				bitmap = new Util::Bitmap(static_cast<uint32_t>(tFormat.width), static_cast<uint32_t>(tFormat.height), Util::PixelFormat::BGR);
			break;
		}
		case GL_BGRA:{
			if(tFormat.glDataType == GL_UNSIGNED_BYTE)
				bitmap = new Util::Bitmap(static_cast<uint32_t>(tFormat.width), static_cast<uint32_t>(tFormat.height), Util::PixelFormat::BGRA);
			break;
		}
#endif /* LIB_GL */
		default:
			break;
	}
	if(bitmap==nullptr){
		WARN("createBitmapFromTexture: The texture format is not supported");
		return nullptr;
	}
	uint8_t * pixels = bitmap->data();
	const uint8_t * textureData = texture->getLocalData();

	std::copy(textureData, textureData + bitmap->getDataSize(), pixels);
	bitmap->flipVertically();

	return bitmap;
}

Util::Reference<Util::PixelAccessor> TextureUtils::createColorPixelAccessor(RenderingContext & context, Texture * texture) {
	texture->openLocalData(context);
	return Util::PixelAccessor::create(texture->getLocalBitmap());
}

Util::Reference<Util::PixelAccessor> TextureUtils::createDepthPixelAccessor(RenderingContext & context, Texture * texture) {
	class DepthAccessor : public Util::PixelAccessor {
		public:
			DepthAccessor(Util::Bitmap * bitmap) : Util::PixelAccessor(bitmap) {
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

			void doWriteColor(uint32_t /*x*/, uint32_t /*y*/, float /*value*/) override {
				throw std::logic_error("Unsupported function called");
			}

			void doWriteColor(uint32_t /*x*/, uint32_t /*y*/, uint8_t /*value*/) override {
				throw std::logic_error("Unsupported function called");
			}
	};
#ifdef LIB_GL
	if(texture->getFormat().glFormat != GL_DEPTH_STENCIL_EXT) {
#endif /* LIB_GL */
		return createColorPixelAccessor(context, texture);
#ifdef LIB_GL
	}
	texture->openLocalData(context);
	return new DepthAccessor(texture->getLocalBitmap());
#endif /* LIB_GL */
}

Util::Reference<Util::PixelAccessor> TextureUtils::createStencilPixelAccessor(RenderingContext & context, Texture * texture) {
	class StencilAccessor : public Util::PixelAccessor {
		public:
			StencilAccessor(Util::Bitmap * bitmap) : Util::PixelAccessor(bitmap) {
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

			void doWriteColor(uint32_t /*x*/, uint32_t /*y*/, float /*value*/) override {
				throw std::logic_error("Unsupported function called");
			}

			void doWriteColor(uint32_t /*x*/, uint32_t /*y*/, uint8_t /*value*/) override {
				throw std::logic_error("Unsupported function called");
			}
	};
#ifdef LIB_GL
	if(texture->getFormat().glFormat != GL_DEPTH_STENCIL_EXT) {
#endif /* LIB_GL */
		return createColorPixelAccessor(context, texture);
#ifdef LIB_GL
	}
	texture->openLocalData(context);
	return new StencilAccessor(texture->getLocalBitmap());
#endif /* LIB_GL */
}

float TextureUtils::minDepthDistance(RenderingContext & context, Texture * firstTex, Texture * secondTex) {
	// check parameter validity
	if(firstTex == nullptr) {
		INVALID_ARGUMENT_EXCEPTION("Texture firstTex may not be nullptr.");
	}
	const uint32_t width = static_cast<const uint32_t>(firstTex->getWidth());
	const uint32_t height = static_cast<const uint32_t>(firstTex->getHeight());
	if(width == 0 || height == 0) {
		INVALID_ARGUMENT_EXCEPTION("Textures may not have a size of 0.");
	}
	if(secondTex == nullptr || width != static_cast<const uint32_t>(secondTex->getWidth()) || height != static_cast<const uint32_t>(secondTex->getHeight())) {
		INVALID_ARGUMENT_EXCEPTION("Texture second may not be nullptr and has to be of the same size as firstTex.");
	}

	// download and open textures
	firstTex->downloadGLTexture(context);
	const float * firstData = reinterpret_cast<const float *>(firstTex->openLocalData(context));
	secondTex->downloadGLTexture(context);
	const float * secondData = reinterpret_cast<const float *>(secondTex->openLocalData(context));

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

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

/*! (static) Factory */
Texture * createStdTexture(uint32_t width, uint32_t height, bool alpha, bool useMipmaps, bool clampToEdge) {
	Texture::Format format;
	format.glTextureType = GL_TEXTURE_2D;
	format.sizeX = width;
	format.sizeY = height;
	format.glFormat = alpha ? GL_RGBA : GL_RGB;
	format.glDataType = GL_UNSIGNED_BYTE;
	format.glInternalFormat=alpha ? GL_RGBA : GL_RGB;
	format.glMagFilter = GL_NEAREST;
	format.glMinFilter = useMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
	format.autoCreateMipmaps = useMipmaps;

	if(clampToEdge) {
		format.glWrapS = GL_CLAMP_TO_EDGE;
		format.glWrapT = GL_CLAMP_TO_EDGE;
		format.glWrapR = GL_CLAMP_TO_EDGE;
	}

	return new Texture(format);
}

Texture * createNoiseTexture(uint32_t width, uint32_t height, bool alpha, bool useMipmaps, float scaling) {
	Texture::Format format;
	format.glTextureType = GL_TEXTURE_2D;
	format.sizeX = width;
	format.sizeY = height;
	format.glFormat = alpha ? GL_RGBA : GL_RGB;
	format.glDataType = GL_UNSIGNED_BYTE;
	format.glInternalFormat = alpha ? GL_RGBA : GL_RGB;
	format.glMagFilter = GL_LINEAR;
	format.glMinFilter = useMipmaps ? GL_NEAREST_MIPMAP_LINEAR : GL_LINEAR;
	format.autoCreateMipmaps = useMipmaps;

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


Texture * createTextureDataArray_Vec4(const uint32_t size) {
#if defined(LIB_GL)
	Texture::Format format;
	format.glTextureType = GL_TEXTURE_1D;
	format.sizeX = size;
	format.sizeY = 1;
	format.glFormat = GL_RGBA;
	format.glDataType = GL_FLOAT;
	format.glInternalFormat = GL_RGBA32F_ARB;
	format.glMagFilter = GL_LINEAR;
	format.glMinFilter = GL_LINEAR;
	format.glWrapS = GL_CLAMP;
	format.glWrapT = GL_CLAMP;
	format.autoCreateMipmaps = false;
	
	return new Texture(format);
#else
	return nullptr;
#endif
}

#ifdef LIB_GL
/*! (static) Factory */
Texture * createHDRTexture(uint32_t width, uint32_t height, bool alpha, bool useMipmaps) {
	Texture::Format format;
	format.glTextureType=GL_TEXTURE_2D;
	format.sizeX = width;
	format.sizeY = height;
	format.glFormat = alpha ? GL_RGBA : GL_RGB;
	format.glDataType = GL_FLOAT;
	format.glInternalFormat = alpha ? GL_RGBA32F_ARB : GL_RGB32F_ARB;
	format.glMagFilter = GL_LINEAR;
	format.glMinFilter = useMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
	format.autoCreateMipmaps = useMipmaps;

	return new Texture(format);
}
/*! (static) Factory */
Texture * createRedTexture(uint32_t width, uint32_t height, bool useByte, bool useMipmaps) {
	Texture::Format format;
	format.glTextureType = GL_TEXTURE_2D;
	format.sizeX = width;
	format.sizeY = height;
	format.glFormat = GL_RED;
	format.glDataType = useByte ? GL_UNSIGNED_BYTE : GL_FLOAT;
	format.glInternalFormat = useByte ? 1 : GL_R32F;
	format.glMagFilter = GL_NEAREST;
	format.glMinFilter = useMipmaps ? GL_NEAREST_MIPMAP_LINEAR : GL_LINEAR;
	format.autoCreateMipmaps = useMipmaps;

	return new Texture(format);
}

/*! (static) Factory */
Texture * createDepthStencilTexture(uint32_t width, uint32_t height) {
	Texture::Format format;
	format.glTextureType = GL_TEXTURE_2D;
	format.sizeX = width;
	format.sizeY = height;
	format.glFormat = GL_DEPTH_STENCIL_EXT;
	format.glDataType = GL_UNSIGNED_INT_24_8_EXT;
	format.glInternalFormat = GL_DEPTH24_STENCIL8_EXT;
	format.glMagFilter = GL_NEAREST;
	format.glMinFilter = GL_NEAREST;
	format.autoCreateMipmaps = false;

	return new Texture(format);
}
#endif

/*! (static) Factory */
Texture * createDepthTexture(uint32_t width, uint32_t height) {
	Texture::Format format;
	format.glTextureType = GL_TEXTURE_2D;
	format.sizeX = width;
	format.sizeY = height;
	format.glFormat = GL_DEPTH_COMPONENT;
	format.glDataType = GL_FLOAT;
	format.glInternalFormat = GL_DEPTH_COMPONENT;
	format.glMagFilter = GL_NEAREST;
	format.glMinFilter = GL_NEAREST;
	format.autoCreateMipmaps = false;

	return new Texture(format);
}

//! [static] Factory
Texture * createChessTexture(uint32_t width, uint32_t height, int fieldSize_powOfTwo, bool useMipmaps) {
	Texture::Format format=Texture::Format();
	format.glTextureType=GL_TEXTURE_2D;
	format.sizeX = width;
	format.sizeY = height;
	format.glFormat=GL_RGBA;
	format.glDataType=GL_UNSIGNED_BYTE;
	format.glInternalFormat=GL_RGBA;
	format.glMagFilter=GL_NEAREST;
	format.glMinFilter=useMipmaps ? GL_NEAREST_MIPMAP_LINEAR : GL_LINEAR;
	format.autoCreateMipmaps = useMipmaps;

	auto t=new Texture(format);
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

Texture * createTextureFromBitmap(const Util::Bitmap & bitmap, bool useMipmaps, bool clampToEdge) {
	const uint32_t height = bitmap.getHeight();
	const uint32_t width = bitmap.getWidth();

	Texture::Format format;
	format.glTextureType = GL_TEXTURE_2D;
	format.glDataType = GL_UNSIGNED_BYTE;
	format.sizeY = height;
	format.sizeX = width;

	const Util::PixelFormat & pixelFormat = bitmap.getPixelFormat();
	if(pixelFormat==Util::PixelFormat::RGBA){
		format.glFormat = GL_RGBA;
		format.glInternalFormat = GL_RGBA;
	}else if(pixelFormat==Util::PixelFormat::RGB){
		format.glFormat = GL_RGB;
		format.glInternalFormat = GL_RGB;
#ifdef LIB_GL
	}else if(pixelFormat==Util::PixelFormat::BGRA){
		format.glFormat = GL_BGRA;
		format.glInternalFormat = GL_RGBA;
	}else if(pixelFormat==Util::PixelFormat::BGR){
		format.glFormat = GL_BGR;
		format.glInternalFormat = GL_RGB;
	}else if(pixelFormat==Util::PixelFormat::MONO){
		format.glFormat = GL_RED;
		format.glInternalFormat = GL_RED;
#endif /* LIB_GL */
	}else{
		WARN("createTextureFromBitmap: Bitmap has unimplemented color format.");
		return nullptr;
	}

	format.glMinFilter = useMipmaps ? GL_NEAREST_MIPMAP_LINEAR : GL_LINEAR;
	format.autoCreateMipmaps = useMipmaps;

	if(clampToEdge) {
		format.glWrapS = GL_CLAMP_TO_EDGE;
		format.glWrapT = GL_CLAMP_TO_EDGE;
		format.glWrapR = GL_CLAMP_TO_EDGE;
	}

	auto texture = new Texture(format);
	texture->allocateLocalData();
	const uint8_t * pixels = bitmap.data();

	// Flip the rows.
	const uint32_t rowSize = width * pixelFormat.getBytesPerPixel();
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
Texture * createTextureFromRAW(const Util::FileName & filename, unsigned int type, bool useMipmaps, bool clampToEdge, bool flip_h) {
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
	format.glTextureType=GL_TEXTURE_2D;
	format.sizeY = width;
	format.sizeX = width;
	format.glDataType=GL_FLOAT;
	format.glInternalFormat=GL_RGB;

	format.glFormat = GL_RGB;
	format.glMinFilter = useMipmaps ? GL_NEAREST_MIPMAP_LINEAR : GL_LINEAR;
	format.autoCreateMipmaps = useMipmaps;
	if(clampToEdge) {
		format.glWrapS = GL_CLAMP_TO_EDGE;
		format.glWrapT = GL_CLAMP_TO_EDGE;
		format.glWrapR = GL_CLAMP_TO_EDGE;
	}
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
Texture * createTextureFromScreen(int xpos, int ypos, const Texture::Format & format) {
	auto texture = new Texture(format);
	texture->allocateLocalData();
	glReadPixels(xpos, ypos, static_cast<GLsizei>(format.sizeX), static_cast<GLsizei>(format.sizeY), format.glFormat, format.glDataType, texture->getLocalData());
	return texture;
}

//! [static]
Texture * createTextureFromScreen(int xpos/*=0*/, int ypos/*=0*/, int width/*=-1*/, int height/*=-1*/,bool useAlpha){
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
	format.glDataType = GL_UNSIGNED_BYTE;
	format.glFormat = useAlpha ? GL_RGBA : GL_RGB;
	return createTextureFromScreen(xpos, ypos, format);
}

//! [static]
void updateTextureFromScreen(RenderingContext & context,Texture * t,const Geometry::Rect_i & textureRect, int screenPosX/*=0*/, int screenPosY/*=0*/){
	const Texture::Format & format=t->getFormat();
	const int width=textureRect.getWidth()>static_cast<int>(format.sizeX) ? static_cast<int>(format.sizeX) : textureRect.getWidth();
	const int height=textureRect.getHeight()>static_cast<int>(format.sizeY) ? static_cast<int>(format.sizeY) : textureRect.getHeight();
	context.pushAndSetTexture(0,t);
	glCopyTexSubImage2D(GL_TEXTURE_2D,0,textureRect.getX(), textureRect.getY(),screenPosX,screenPosY, width, height);
	context.popTexture(0);
	GET_GL_ERROR();
}

//! [static]
void updateTextureFromScreen(RenderingContext & context,Texture * t){
	updateTextureFromScreen(context,t,Geometry::Rect_i(0,0,t->getFormat().sizeX,t->getFormat().sizeY));
}

#ifdef LIB_GL

//! [static]
void drawTextureToScreen(RenderingContext&rc,const Geometry::Rect_i & screenRect,Texture * t,const Geometry::Rect_f & textureRect){
	if(!t)
		return;
	std::vector<Texture *> textures;
	textures.push_back(t);
	std::vector<Geometry::Rect_f> rects;
	rects.push_back(textureRect);

	drawTextureToScreen(rc,screenRect,textures,rects);
}

//! (static)
void drawTextureToScreen(RenderingContext & rc, const Geometry::Rect_i & screenRect, const std::vector<Texture *> & textures,
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

Util::Reference<Util::Bitmap> createBitmapFromTexture(RenderingContext & context,Texture * texture) {
	if(texture == nullptr){
		WARN("Error creating bitmap: texture was null");
		return nullptr;
	}
	if(texture->getLocalData() == nullptr){
		if(!texture->isGLTextureValid()){
			WARN("Error creating bitmap: texture has no local data and gl data invalid");
			return nullptr;
		}
		texture->downloadGLTexture(context);
	}
	return std::move(createBitmapFromLocalTexture(texture));
}

Util::Reference<Util::Bitmap> createBitmapFromLocalTexture(Texture * texture) {
	if(texture == nullptr) {
		return nullptr;
	}
	if(texture->getLocalData() == nullptr) {
		WARN("Texture has no local data; can not create Bitmap.");
		return nullptr;
	}

	const Texture::Format & format = texture->getFormat();

	if(format.glTextureType != GL_TEXTURE_2D) {
		WARN("createBitmapFromTexture: Other texture types than GL_TEXTURE_2D are not supported.");
		return nullptr;
	}

	Util::Reference<Util::Bitmap> bitmap;

	switch (format.glFormat) {
#ifdef LIB_GL
		case GL_RED:
		case GL_GREEN:
		case GL_BLUE:
		case GL_ALPHA:
#endif /* LIB_GL */
		case GL_DEPTH_COMPONENT:{
			if(format.glDataType == GL_UNSIGNED_BYTE){
				bitmap = new Util::Bitmap(static_cast<uint32_t>(format.sizeX), static_cast<uint32_t>(format.sizeY), Util::PixelFormat::MONO);
			}else if(format.glDataType == GL_FLOAT){
				bitmap = new Util::Bitmap(static_cast<uint32_t>(format.sizeX), static_cast<uint32_t>(format.sizeY), Util::PixelFormat::MONO_FLOAT);
			}
			break;
		}
		case GL_RGB:{
			if(format.glDataType == GL_UNSIGNED_BYTE)
				bitmap = new Util::Bitmap(static_cast<uint32_t>(format.sizeX), static_cast<uint32_t>(format.sizeY), Util::PixelFormat::RGB);
			break;
		}
		case GL_RGBA:{
			if(format.glDataType == GL_UNSIGNED_BYTE)
				bitmap = new Util::Bitmap(static_cast<uint32_t>(format.sizeX), static_cast<uint32_t>(format.sizeY), Util::PixelFormat::RGBA);
			break;
		}
#ifdef LIB_GL
		case GL_BGR:{
			if(format.glDataType == GL_UNSIGNED_BYTE)
				bitmap = new Util::Bitmap(static_cast<uint32_t>(format.sizeX), static_cast<uint32_t>(format.sizeY), Util::PixelFormat::BGR);
			break;
		}
		case GL_BGRA:{
			if(format.glDataType == GL_UNSIGNED_BYTE)
				bitmap = new Util::Bitmap(static_cast<uint32_t>(format.sizeX), static_cast<uint32_t>(format.sizeY), Util::PixelFormat::BGRA);
			break;
		}
#endif /* LIB_GL */
		default:
			break;
	}
	if(bitmap.isNull()){
		WARN("createBitmapFromTexture: The texture format is not supported");
		return nullptr;
	}
	uint8_t * pixels = bitmap->data();
	const uint8_t * textureData = texture->getLocalData();

	std::copy(textureData, textureData + bitmap->getDataSize(), pixels);
	bitmap->flipVertically();

	return std::move(bitmap);
}

Util::Reference<Util::PixelAccessor> createColorPixelAccessor(RenderingContext & context, Texture * texture) {
	texture->openLocalData(context);
	return Util::PixelAccessor::create(texture->getLocalBitmap());
}

Util::Reference<Util::PixelAccessor> createDepthPixelAccessor(RenderingContext & context, Texture * texture) {
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

Util::Reference<Util::PixelAccessor> createStencilPixelAccessor(RenderingContext & context, Texture * texture) {
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

float minDepthDistance(RenderingContext & context, Texture * firstTex, Texture * secondTex) {
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
}

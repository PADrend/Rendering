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
#include "../Core/Sampler.h"
#include "../Core/ImageView.h"
#include "../Core/ImageStorage.h"
#include "../Core/Swapchain.h"
#include "../Core/CommandBuffer.h"
#include "../Mesh/Mesh.h"
#include "../Mesh/MeshDataStrategy.h"
#include "../Mesh/MeshIndexData.h"
#include "../Mesh/MeshVertexData.h"
#include "../Mesh/VertexAttribute.h"
#include "../Mesh/VertexAttributeIds.h"
#include "../Mesh/VertexDescription.h"
#include "../RenderingContext/RenderingParameters.h"
#include "../RenderingContext/RenderingContext.h"
#include "../Helper.h"
#include "../FBO.h"
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
#include <Util/UI/Window.h>
#include <Util/Macros.h>
#include <Util/References.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace Rendering {
namespace TextureUtils {

Util::AttributeFormat glPixelFormatToPixelFormat(const PixelFormatGL& glPixelFormat) { return {}; }

// ----------------------------------------------------------------------------
// factory functions

static Texture::Ref create(const Device::Ref& device,  TextureType type,uint32_t sizeX,uint32_t sizeY,uint32_t numLayers,InternalFormat internalFormat, bool filtering,bool clampToEdge=false,uint32_t samples=1){
	Texture::Format format;
	format.pixelFormat = internalFormat;
	switch(type) {
		case TextureType::TEXTURE_1D:
			format.extent = {sizeX, 1, 1};
			format.layers = 1;
			break;
		case TextureType::TEXTURE_2D:
			format.extent = {sizeX, sizeY, 1};
			format.layers = 1;
			break;
		case TextureType::TEXTURE_3D:
			format.extent = {sizeX, sizeY, numLayers};
			format.layers = 1;
			break;
		case TextureType::TEXTURE_1D_ARRAY:
			format.extent = {sizeX, 1, 1};
			format.layers = numLayers;
			break;
		case TextureType::TEXTURE_2D_ARRAY:
			format.extent = {sizeX, sizeY, 1};
			format.layers = numLayers;
			break;
		case TextureType::TEXTURE_CUBE_MAP:
			format.extent = {sizeX, sizeY, 6};
			format.layers = 1;
			break;
		case TextureType::TEXTURE_CUBE_MAP_ARRAY:
			format.extent = {sizeX, sizeY, 6};
			format.layers = numLayers;
			break;
		default:
			WARN("TextureUtils::create: unsupported texture type: " + getTypeString(type));
	}
	format.samples = samples;

	Sampler::Configuration samplerConfig{};
	samplerConfig.magFilter = filtering ? ImageFilter::Linear : ImageFilter::Nearest;
	samplerConfig.minFilter = filtering ? ImageFilter::Linear : ImageFilter::Nearest;
	samplerConfig.mipmapMode = filtering ? ImageFilter::Linear : ImageFilter::Nearest;
	samplerConfig.addressModeU = clampToEdge ? ImageAddressMode::ClampToEdge : ImageAddressMode::Repeat;
	samplerConfig.addressModeV = clampToEdge ? ImageAddressMode::ClampToEdge : ImageAddressMode::Repeat;
	samplerConfig.addressModeW = clampToEdge ? ImageAddressMode::ClampToEdge : ImageAddressMode::Repeat;

	auto sampler = Sampler::create(device, samplerConfig);	
	return Texture::create(device, format, sampler);
}

//! [static] Factory
Texture::Ref createColorTexture(const Device::Ref& device, TextureType type,uint32_t sizeX,uint32_t sizeY, uint32_t numLayers, Util::TypeConstant dataType, uint8_t numComponents,bool filtering,bool clampToEdge/*=false*/,uint32_t samples){
	if( numComponents<1||numComponents>4 )
		throw std::logic_error("createTexture: Invalid numComponents.");
	auto pixelFormat = toInternalFormat( Util::AttributeFormat({"COLOR"}, dataType, numComponents));
	return create(device, type, sizeX, sizeY, numLayers, pixelFormat, filtering, clampToEdge, samples);
}

//! (static)
Texture::Ref createStdCubeTexture(const Device::Ref& device, uint32_t width, bool alpha) {
	return createColorTexture(device, TextureType::TEXTURE_CUBE_MAP, width, width, 6, Util::TypeConstant::UINT8, alpha ? 4 : 3,  true);
}

//! (static)
Texture::Ref createStdTexture(const Device::Ref& device, uint32_t width, uint32_t height, bool alpha) {
	return createColorTexture(device, TextureType::TEXTURE_2D, width, height, 1, Util::TypeConstant::UINT8, alpha ? 4 : 3,  true);
}

//! (static)
Texture::Ref createHDRCubeTexture(const Device::Ref& device, uint32_t width, bool alpha) {
	return createColorTexture(device, TextureType::TEXTURE_CUBE_MAP, width, width, 6, Util::TypeConstant::FLOAT, alpha ? 4 : 3, true);
}

//! (static)
Texture::Ref createHDRTexture(const Device::Ref& device, uint32_t width, uint32_t height, bool alpha) {
	return createColorTexture(device, TextureType::TEXTURE_2D, width, height, 1, Util::TypeConstant::FLOAT, alpha ? 4 : 3, true);
}

//! (static)
Texture::Ref createRedTexture(const Device::Ref& device, uint32_t width, uint32_t height, bool useByte) {
	return createColorTexture(device, TextureType::TEXTURE_2D, width, height, 1, useByte ? Util::TypeConstant::UINT8 : Util::TypeConstant::FLOAT, 1,  true);
}

//! (static)
Texture::Ref createDepthStencilTexture(const Device::Ref& device, uint32_t width, uint32_t height) {
	return create(device, TextureType::TEXTURE_2D, width, height, 1, InternalFormat::D24UnormS8, false, true);
}

//! [static] Factory
Texture::Ref createDepthTexture(const Device::Ref& device, uint32_t width, uint32_t height, uint32_t layers) {
	return create(device, TextureType::TEXTURE_2D_ARRAY, width, height, layers, InternalFormat::D24UnormS8, false, true);
}

//! [static] Factory
Texture::Ref createHDRDepthTexture(const Device::Ref& device, uint32_t width, uint32_t height, uint32_t layers) {
	return create(device, TextureType::TEXTURE_2D_ARRAY, width, height, layers, InternalFormat::D32Float, false, true);
}


//! [static] Factory
Texture::Ref createMultisampleDepthTexture(const Device::Ref& device, uint32_t width, uint32_t height, uint32_t samples) {
	return create(device, TextureType::TEXTURE_2D_MULTISAMPLE, width, height, 1, InternalFormat::D24UnormS8, false, true, samples);
}

//! (static)
Texture::Ref createMultisampleTexture(const Device::Ref& device, uint32_t width, uint32_t height, bool alpha, uint32_t samples) {
	return createColorTexture(device, TextureType::TEXTURE_2D_MULTISAMPLE, width, height, 1, Util::TypeConstant::UINT8, alpha ? 4 : 3, true, samples);
}

//! [static] Factory
Texture::Ref createDataTexture(const Device::Ref& device, TextureType type,uint32_t sizeX,uint32_t sizeY, uint32_t numLayers, Util::TypeConstant dataType, uint8_t numComponents){
	return createColorTexture(device, type,sizeX,sizeY,numLayers,dataType,numComponents,false);
}
// ----------------------------

Texture::Ref createNoiseTexture(const Device::Ref& device, uint32_t width, uint32_t height, bool alpha,float scaling) {
	Texture::Ref texture = create(device, TextureType::TEXTURE_2D,width,height,1, alpha ? InternalFormat::RGBA32Float : InternalFormat::RGB32Float, true, false);

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

	return texture;
}

Texture::Ref createTextureDataArray_Vec4(const Device::Ref& device, const uint32_t size) {
	return createDataTexture(device, TextureType::TEXTURE_1D,size,1,1,Util::TypeConstant::FLOAT,4); 
}

Texture::Ref createColorPalette(const Device::Ref& device, const std::vector<Util::Color4f>& colors) {
	if(colors.empty()) {
		WARN("createColorPalette: invalid number of colors!");
		return nullptr;
	}
	auto t = create(device, TextureType::TEXTURE_1D, colors.size(), 1, 1, InternalFormat::RGBA8Unorm, false, true);
	t->allocateLocalData();
	auto acc = Util::PixelAccessor::create(t->getLocalBitmap());
	for(uint32_t i=0; i<colors.size(); ++i) {
		acc->writeColor(i, 0, colors[i]);
	}
	t->dataChanged();
	return t;
}

//! [static] Factory
Texture::Ref createChessTexture(const Device::Ref& device, uint32_t width, uint32_t height, int fieldSize_powOfTwo, bool filtering, bool clampToEdge) {
	auto t = create(device, TextureType::TEXTURE_2D, width, height, 1, InternalFormat::RGBA8Unorm, filtering, clampToEdge);

	t->allocateLocalData();
	uint8_t* tData = t->getLocalData();

	uint8_t c;
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

Texture::Ref createTextureFromBitmap(const Device::Ref& device, const Util::Bitmap& bitmap, TextureType type, uint32_t numLayers, bool clampToEdge) {
	const uint32_t bHeight = bitmap.getHeight();
	const uint32_t width = bitmap.getWidth();

	WARN_AND_RETURN_IF(numLayers==0 || numLayers>bHeight || (bHeight%numLayers) != 0, "createTextureFromBitmap: Bitmap height is not dividable into given number of layers.", nullptr);

	auto pixelFormat = toInternalFormat(bitmap.getPixelFormat());
	WARN_AND_RETURN_IF(pixelFormat == InternalFormat::Unknown, "createTextureFromBitmap: Bitmap has unimplemented pixel format.", nullptr);
	
	auto texture = create(device, type, width, bHeight/numLayers, numLayers, pixelFormat, true, clampToEdge);
	texture->allocateLocalData();
	const uint8_t * pixels = bitmap.data();

	// Flip the rows.
	const uint32_t rowSize = width * bitmap.getPixelFormat().getDataSize();
	for (uint_fast16_t row = 0; row < bHeight; ++row) {
		const uint32_t offset = row * rowSize;
		const uint16_t reverseRow = bHeight - 1 - row;
		const uint32_t reverseOffset = reverseRow * rowSize;
		std::copy(pixels + reverseOffset, pixels + reverseOffset + rowSize, texture->getLocalData() + offset);
	}

	texture->dataChanged();
	return texture;
}

//! [static]
Texture::Ref createTextureFromScreen(const Device::Ref& device, int xpos, int ypos, const Texture::Format & format) {
	auto texture = Texture::create(device, format);
	Geometry::Rect_i rect{0, 0, static_cast<int32_t>(format.extent.getX()), static_cast<int32_t>(format.extent.getY())};
	updateTextureFromScreen(device, *texture.get(), rect, xpos, ypos);
	return texture;
}

//! [static]
Texture::Ref createTextureFromScreen(const Device::Ref& device, int xpos/*=0*/, int ypos/*=0*/, int width/*=-1*/, int height/*=-1*/,bool useAlpha) {
	if(width < 0 || height < 0) {
	if(width<0)
		width = device->getWindow()->getWidth()-xpos;
	if(height<0)
		height = device->getWindow()->getHeight()-ypos;
	}
	Texture::Format format{};
	format.extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
	format.pixelFormat = InternalFormat::RGBA8Unorm;
	
	return createTextureFromScreen(device, xpos, ypos, format);
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

	if(f1 != f2) {
		return false;
	}

	return std::equal(t1->getLocalData(), t1->getLocalData() + t1->getDataSize(), t2->getLocalData());
}

void updateTextureFromScreen(const Device::Ref& device,Texture & t,const Geometry::Rect_i & textureRect, int screenPosX/*=0*/, int screenPosY/*=0*/) {
	t.upload(); // make sure texture is uploaded
	auto swapchain = device->getSwapchain();
	auto srcFBO = swapchain->getFBO((swapchain->getCurrentIndex() + swapchain->getSize() - 1) % swapchain->getSize()); // get last swapchain image

	ImageRegion srcRegion{};
	srcRegion.extent = {static_cast<uint32_t>(textureRect.getWidth()), static_cast<uint32_t>(textureRect.getHeight()), 1};
	srcRegion.offset = {screenPosX, screenPosY, 0};
	ImageRegion tgtRegion{};
	tgtRegion.extent = {static_cast<uint32_t>(textureRect.getWidth()), static_cast<uint32_t>(textureRect.getHeight()), 1};
	tgtRegion.offset = {textureRect.getX(), textureRect.getY(), 0};
	
	CommandBuffer::Ref cmds = CommandBuffer::create(device->getQueue(QueueFamily::Transfer));
	cmds->blitImage(srcFBO->getColorAttachment()->getImage(), t.getImage(), srcRegion, tgtRegion, ImageFilter::Nearest);
	cmds->submit(true);
}

//! [static]
void updateTextureFromScreen(RenderingContext & context,Texture & t,const Geometry::Rect_i & textureRect, int screenPosX/*=0*/, int screenPosY/*=0*/) {
	updateTextureFromScreen(context.getDevice(), t, textureRect, screenPosX, screenPosY);
}

//! [static]
void updateTextureFromScreen(RenderingContext & context,Texture & t){
	updateTextureFromScreen(context,t,Geometry::Rect_i(0,0,t.getWidth(),t.getHeight()));
}
//! [static]
void drawTextureToScreen(RenderingContext& rc, const Geometry::Rect_i & screenRect, Texture & t, const Geometry::Rect_f & textureRect) {
	
	std::vector<Texture *> textures;
	textures.push_back(&t);
	std::vector<Geometry::Rect_f> rects;
	rects.push_back(textureRect);

	drawTextureToScreen(rc,screenRect,textures,rects);
}

//! (static)
void drawTextureToScreen(RenderingContext & rc, const Geometry::Rect_i & screenRect, const std::vector<Texture *> & textures, const std::vector<Geometry::Rect_f> & textureRects) {
	
	uint8_t numTextures = textures.size() < textureRects.size() ? textures.size() : textureRects.size();
	if(numTextures == 0) {
		return;
	}
	if(numTextures > 8) {
		WARN("At most eight textures are supported.");
		numTextures = 8;
	}
	auto depthState = rc.getDepthStencil();
	depthState.setDepthTestEnabled(false);
	depthState.setDepthWriteEnabled(false);
	rc.pushAndSetDepthStencil(depthState);
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
		texCoordAttr.push_back(vertexDesc.appendTexCoord(i));
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
	rc.popDepthStencil();	
}

Util::Reference<Util::Bitmap> createBitmapFromTexture(Texture & texture) {
	if(texture.getLocalData() == nullptr){
		if(!texture.isValid()){
			WARN("Error creating bitmap: texture has no local data and gl data invalid");
			return nullptr;
		}
		texture.download();
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

Util::Reference<Util::PixelAccessor> createColorPixelAccessor(Texture& texture) {
	texture.openLocalData();
	return Util::PixelAccessor::create(texture.getLocalBitmap());
}

Util::Reference<Util::PixelAccessor> createDepthPixelAccessor(Texture& texture) {
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
	
	if(!isDepthStencilFormat(texture.getFormat())) {
		return createColorPixelAccessor(texture);
	}
	texture.openLocalData();
	return new DepthAccessor(texture.getLocalBitmap());
}

Util::Reference<Util::PixelAccessor> createStencilPixelAccessor(Texture& texture) {
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
	if(!isDepthStencilFormat(texture.getFormat())) {
		return createColorPixelAccessor(texture);
	}
	texture.openLocalData();
	return new StencilAccessor(texture.getLocalBitmap());
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
	firstTex.download();
	const float * firstData = reinterpret_cast<const float *>(firstTex.openLocalData(context));
	secondTex.download();
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

Util::Reference<Util::Bitmap> createBitmapFromTexture(RenderingContext& context, Texture& texture) {
	return createBitmapFromTexture(texture);
}

Util::Reference<Util::PixelAccessor> createColorPixelAccessor(RenderingContext & context, Texture& texture) {
	return createColorPixelAccessor(texture);
}

Util::Reference<Util::PixelAccessor> createDepthPixelAccessor(RenderingContext & context, Texture& texture) {
	return createDepthPixelAccessor(texture);
}

Util::Reference<Util::PixelAccessor> createStencilPixelAccessor(RenderingContext & context, Texture& texture) {
	return createStencilPixelAccessor(texture);
}

}
}

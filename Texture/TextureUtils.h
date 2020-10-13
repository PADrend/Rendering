/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2014 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_TEXTUREUTILS_H
#define RENDERING_TEXTUREUTILS_H

#include "../Core/Device.h"
#include "Texture.h"
#include "PixelFormatGL.h"
#include <Util/References.h>
#include <Util/TypeConstant.h>
#include <cstdint>
#include <vector>

namespace Geometry {
template<typename _T> class _Rect;
typedef _Rect<int> Rect_i;
typedef _Rect<float> Rect_f;
}

namespace Util {
class Bitmap;
class FileName;
class PixelAccessor;
class AttributeFormat;
}

namespace Rendering {
class RenderingContext;

/** Collection of texture related operations.
 * @ingroup texture
 */
namespace TextureUtils {

const unsigned int RAW_16BIT_BW = 0;

[[deprecated]]
static PixelFormatGL pixelFormatToGLPixelFormat(const Util::AttributeFormat & pixelFormat) { return {}; }
[[deprecated]]
Util::AttributeFormat glPixelFormatToPixelFormat(const PixelFormatGL& glPixelFormat);
[[deprecated]]
static uint32_t textureTypeToGLTextureType(TextureType type) { return 0; }

Texture::Ref createStdCubeTexture(const Device::Ref& device, uint32_t width, bool alpha);
[[deprecated]]
static Texture::Ref createStdCubeTexture(uint32_t width, bool alpha) { return createStdCubeTexture(Device::getDefault(), width, alpha); }

Texture::Ref createHDRCubeTexture(const Device::Ref& device, uint32_t width, bool alpha);
[[deprecated]]
static Texture::Ref createHDRCubeTexture(uint32_t width, bool alpha) { return createHDRCubeTexture(Device::getDefault(), width, alpha); }

Texture::Ref createStdTexture(const Device::Ref& device, uint32_t width, uint32_t height, bool alpha);
[[deprecated]]
static Texture::Ref createStdTexture(uint32_t width, uint32_t height, bool alpha) { return createStdTexture(Device::getDefault(), width, height, alpha); }

Texture::Ref createNoiseTexture(const Device::Ref& device, uint32_t width, uint32_t height, bool alpha, float scaling = 1.0f);
[[deprecated]]
static Texture::Ref createNoiseTexture(uint32_t width, uint32_t height, bool alpha, float scaling = 1.0f) { return createNoiseTexture(Device::getDefault(), width, height, alpha, scaling); }

Texture::Ref createHDRTexture(const Device::Ref& device, uint32_t width, uint32_t height, bool alpha);
[[deprecated]]
static Texture::Ref createHDRTexture(uint32_t width, uint32_t height, bool alpha) { return createHDRTexture(Device::getDefault(), width, height, alpha); }

Texture::Ref createRedTexture(const Device::Ref& device, uint32_t width, uint32_t height, bool useByte = false);
[[deprecated]]
static Texture::Ref createRedTexture(uint32_t width, uint32_t height, bool useByte = false) { return createRedTexture(Device::getDefault(), width, height, useByte); }

Texture::Ref createDepthStencilTexture(const Device::Ref& device, uint32_t width, uint32_t height);
[[deprecated]]
static Texture::Ref createDepthStencilTexture(uint32_t width, uint32_t height) { return createDepthStencilTexture(Device::getDefault(), width, height); }

Texture::Ref createDepthTexture(const Device::Ref& device, uint32_t width, uint32_t height, uint32_t layers=0);
[[deprecated]]
static Texture::Ref createDepthTexture(uint32_t width, uint32_t height, uint32_t layers=0) { return createDepthTexture(Device::getDefault(), width, height, layers); }

Texture::Ref createHDRDepthTexture(const Device::Ref& device, uint32_t width, uint32_t height, uint32_t layers=0);
[[deprecated]]
static Texture::Ref createHDRDepthTexture(uint32_t width, uint32_t height, uint32_t layers=0) { return createHDRDepthTexture(Device::getDefault(), width, height, layers); }

Texture::Ref createMultisampleDepthTexture(const Device::Ref& device, uint32_t width, uint32_t height, uint32_t samples=4);
[[deprecated]]
static Texture::Ref createMultisampleDepthTexture(uint32_t width, uint32_t height, uint32_t samples=4) { return createMultisampleDepthTexture(Device::getDefault(), width, height, samples); }

Texture::Ref createMultisampleTexture(const Device::Ref& device, uint32_t width, uint32_t height, bool alpha, uint32_t samples=4);
[[deprecated]]
static Texture::Ref createMultisampleTexture(uint32_t width, uint32_t height, bool alpha, uint32_t samples=4) { return createMultisampleTexture(Device::getDefault(), width, height, alpha, samples); }


/*! @p TextureType == TextureType::TEXTURE_1D/2D/3D/1D_ARRAY/2D_ARRAY/CUBE_MAP
	@p numComponents == 1 || 2 || 3|| 4
	@p dataType == FLOAT || UINT8 || UINT32 || INT32
	@note If the texture should be used as image(load and store), only 1,2,or 4 components are valid.
*/
Texture::Ref createColorTexture(const Device::Ref& device, TextureType type,uint32_t sizeX,uint32_t sizeY, uint32_t numLayers, Util::TypeConstant dataType, uint8_t numComponents,bool filtering,bool clampToEdge=false,uint32_t samples=4);
[[deprecated]]
static Texture::Ref createColorTexture(TextureType type,uint32_t sizeX,uint32_t sizeY, uint32_t numLayers, Util::TypeConstant dataType, uint8_t numComponents,bool filtering,bool clampToEdge=false,uint32_t samples=4) {
	return createColorTexture(Device::getDefault(), type,sizeX,sizeY, numLayers, dataType, numComponents,filtering,clampToEdge,samples);
}

/*! @p numComponents == 1 || 2 || 3|| 4
	@p dataType == FLOAT || UINT8 || UINT32 || INT32
	@note no filtering is performed
	@note If the texture should be used as image(load and store), only 1,2,or 4 components are valid.
*/
Texture::Ref createDataTexture(const Device::Ref& device, TextureType type,uint32_t sizeX,uint32_t sizeY, uint32_t numLayers, Util::TypeConstant dataType, uint8_t numComponents);
[[deprecated]]
static Texture::Ref createDataTexture(TextureType type,uint32_t sizeX,uint32_t sizeY, uint32_t numLayers, Util::TypeConstant dataType, uint8_t numComponents) {
	return createDataTexture(Device::getDefault(), type,sizeX,sizeY, numLayers, dataType, numComponents);
}

// creates an vec4 data array as textures for handling big arrays inside shaders. See SkeletalAnimationUtils for generic accessor.
Texture::Ref createTextureDataArray_Vec4(const Device::Ref& device, const uint32_t size);
[[deprecated]]
static Texture::Ref createTextureDataArray_Vec4(const uint32_t size) {
	return createTextureDataArray_Vec4(Device::getDefault(), size);
}

Texture::Ref createChessTexture(const Device::Ref& device, uint32_t width, uint32_t height, int fieldSize_powOfTwo=8, bool filtering=true, bool clampToEdge=false);
[[deprecated]]
static Texture::Ref createChessTexture(uint32_t width, uint32_t height, int fieldSize_powOfTwo=8) {
	return createChessTexture(Device::getDefault(), width, height, fieldSize_powOfTwo);
}

Texture::Ref createColorPalette(const Device::Ref& device, const std::vector<Util::Color4f>& colors);
[[deprecated]]
static Texture::Ref createColorPalette(const std::vector<Util::Color4f>& colors) {
	return createColorPalette(Device::getDefault(), colors);
}

/*! Create a texture of the given @p textureType from the given @p bitmap.

	- For textureType TEXTURE_1D and TEXTURE_2D, numLayers must be 1.
	- For textureType TEXTURE_CUBE_MAP, numLayers must be 6.
	- For textureType TEXTURE_CUBE_MAP_ARRAY, numLayers must be a multiple of 6.	*/
Texture::Ref createTextureFromBitmap(const Device::Ref& device, const Util::Bitmap & bitmap, TextureType type = TextureType::TEXTURE_2D, uint32_t numLayers=1, bool clampToEdge = false);
[[deprecated]]
static Texture::Ref createTextureFromBitmap(const Util::Bitmap & bitmap, TextureType type = TextureType::TEXTURE_2D, uint32_t numLayers=1, bool clampToEdge = false) {
	return createTextureFromBitmap(Device::getDefault(), bitmap, type, numLayers, clampToEdge);
}

Texture::Ref createTextureFromScreen(const Device::Ref& device, int xpos, int ypos, const Texture::Format & format);
[[deprecated]]
static Texture::Ref createTextureFromScreen(int xpos, int ypos, const Texture::Format & format) {
	return createTextureFromScreen(Device::getDefault(), xpos, ypos, format);
}

Texture::Ref createTextureFromScreen(const Device::Ref& device, int xpos=0, int ypos=0, int width=-1, int height=-1,bool useAlpha = true);
[[deprecated]]
static Texture::Ref createTextureFromScreen(int xpos=0, int ypos=0, int width=-1, int height=-1,bool useAlpha = true) {
	return createTextureFromScreen(Device::getDefault(), xpos, ypos, width, height, useAlpha);
}

void updateTextureFromScreen(const Device::Ref& device, Texture& t, const Geometry::Rect_i& textureRect, int screenPosX=0, int screenPosY=0);
void updateTextureFromScreen(const Device::Ref& device, Texture& t);
void updateTextureFromScreen(RenderingContext& context, Texture& t, const Geometry::Rect_i& textureRect, int screenPosX=0, int screenPosY=0);
void updateTextureFromScreen(RenderingContext& context, Texture& t);
void drawTextureToScreen(RenderingContext& rc, const Geometry::Rect_i & screenRect, Texture& t, const Geometry::Rect_f& textureRect);
void drawTextureToScreen(RenderingContext& rc, const Geometry::Rect_i & screenRect, const std::vector<Texture *>& textures, const std::vector<Geometry::Rect_f>& textureRects);

bool compareTextures(Texture *t1, Texture *t2);

//! the texture is downloaded to memory (if necessary), the proper Util-color format is chosen and the texture is flipped vertically.
Util::Reference<Util::Bitmap> createBitmapFromTexture(Texture& texture);
[[deprecated]]
Util::Reference<Util::Bitmap> createBitmapFromTexture(RenderingContext& context, Texture& texture);

//! like createBitmapFromTexture, but the texture is NOT downloaded, but a warning is issued if it should have been.
Util::Reference<Util::Bitmap> createBitmapFromLocalTexture(const Texture & texture);

//! Create a standard pixel accessor for reading color values.
Util::Reference<Util::PixelAccessor> createColorPixelAccessor(Texture& texture);
[[deprecated]]
Util::Reference<Util::PixelAccessor> createColorPixelAccessor(RenderingContext & context, Texture& texture);

//! Create a special pixel accessor for reading depth values. This has to be used for packed depth and stencil image formats.
Util::Reference<Util::PixelAccessor> createDepthPixelAccessor(Texture& texture);
[[deprecated]]
Util::Reference<Util::PixelAccessor> createDepthPixelAccessor(RenderingContext & context, Texture& texture);

//! Create a special pixel accessor for reading stencil values. This has to be used for packed depth and stencil image formats.
Util::Reference<Util::PixelAccessor> createStencilPixelAccessor(Texture& texture);
[[deprecated]]
Util::Reference<Util::PixelAccessor> createStencilPixelAccessor(RenderingContext & context, Texture& texture);

/**
	* Compares two depth texture and determines their minimal distance.
	* Note: This method is heavily customized to support the snapping functionality implemented by getNodeToSceneDistance in scripts/Util/Picking_Utils.escipt.
	* For the comparison the second texture is flipped horizontally and it's values are inverted.
	* In order to have a sound definition of a (positive) distance the implementation checks that the first texture lies completely behind the second one.
	* If this is not the case -1.0f is returned.
	* In case the two textures are disjoint (they don't have a common pixel with a depth value unequal to the clearDepth-value) the method returns -2.0f.
	*/
float minDepthDistance(RenderingContext& context, Texture& firstTex, Texture& secondTex);

}

}

#endif // RENDERING_TEXTUREUTILS_H

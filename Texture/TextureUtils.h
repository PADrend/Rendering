/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_TEXTUREUTILS_H
#define RENDERING_TEXTUREUTILS_H

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
class PixelFormat;
}

namespace Rendering {
class RenderingContext;

//! Collection of texture related operations.
namespace TextureUtils {

const unsigned int RAW_16BIT_BW = 0;

PixelFormatGL pixelFormatToGLPixelFormat(const Util::PixelFormat & pixelFormat);
uint32_t textureTypeToGLTextureType(TextureType type);

Util::Reference<Texture> createStdCubeTexture(uint32_t width, bool alpha);
Util::Reference<Texture> createHDRCubeTexture(uint32_t width, bool alpha);
Util::Reference<Texture> createStdTexture(uint32_t width, uint32_t height, bool alpha);
Util::Reference<Texture> createNoiseTexture(uint32_t width, uint32_t height, bool alpha, float scaling = 1.0f);
Util::Reference<Texture> createHDRTexture(uint32_t width, uint32_t height, bool alpha);
Util::Reference<Texture> createRedTexture(uint32_t width, uint32_t height, bool useByte = false);
Util::Reference<Texture> createDepthStencilTexture(uint32_t width, uint32_t height);
Util::Reference<Texture> createDepthTexture(uint32_t width, uint32_t height);

/*! @p numComponents == 1 || 2 || 3|| 4
	@p dataType == FLOAT || UINT8 || UINT32 || INT32
	@note no filtering is performed
	@note If the texture should be used as image(load and store), only 1,2,or 4 components are valid.
*/
Util::Reference<Texture> createDataTexture(TextureType type,uint32_t sizeX,uint32_t sizeY, uint32_t numLayers, Util::TypeConstant dataType, uint8_t numComponents);

// creates an vec4 data array as textures for handling big arrays inside shaders. See SkeletalAnimationUtils for generic accessor.
Util::Reference<Texture> createTextureDataArray_Vec4(const uint32_t size);
Util::Reference<Texture> createChessTexture(uint32_t width, uint32_t height, int fieldSize_powOfTwo=8);

/*! Create a texture of the given @p textureType from the given @p bitmap.
	- For textureType TEXTURE_1D and TEXTURE_2D, numLayers must be 1.
	- For textureType TEXTURE_CUBE_MAP, numLayers must be 6.
	- For textureType TEXTURE_CUBE_MAP_ARRAY, numLayers must be a multiple of 6.	*/
Util::Reference<Texture> createTextureFromBitmap(const Util::Bitmap & bitmap, TextureType type = TextureType::TEXTURE_2D, uint32_t numLayers=1, bool clampToEdge = false);
Util::Reference<Texture> createTextureFromRAW(const Util::FileName & filename,unsigned int type=RAW_16BIT_BW, bool flip_h = true);
Util::Reference<Texture> createTextureFromScreen(int xpos, int ypos, const Texture::Format & format);
Util::Reference<Texture> createTextureFromScreen(int xpos=0, int ypos=0, int width=-1, int height=-1,bool useAlpha = true);

void updateTextureFromScreen(RenderingContext & context,Texture& t,const Geometry::Rect_i & textureRect, int screenPosX=0, int screenPosY=0);
void updateTextureFromScreen(RenderingContext & context,Texture& t);
void drawTextureToScreen(RenderingContext&rc,const Geometry::Rect_i & screenRect,Texture& t,const Geometry::Rect_f & textureRect);
void drawTextureToScreen(RenderingContext & rc,
						 const Geometry::Rect_i & screenRect,
						 const std::vector<Texture *> & textures,
						 const std::vector<Geometry::Rect_f> & textureRects);

bool compareTextures(Texture *t1, Texture *t2);

//! the texture is downloaded to memory (if necessary), the proper Util-color format is chosen and the texture is flipped vertically.
Util::Reference<Util::Bitmap> createBitmapFromTexture(RenderingContext & context,Texture & texture);

//! like createBitmapFromTexture, but the texture is NOT downloaded, but a warning is issued if it should have been.
Util::Reference<Util::Bitmap> createBitmapFromLocalTexture(const Texture & texture);

//! Create a standard pixel accessor for reading color values.
Util::Reference<Util::PixelAccessor> createColorPixelAccessor(RenderingContext & context, Texture& texture);

//! Create a special pixel accessor for reading depth values. This has to be used for packed depth and stencil image formats.
Util::Reference<Util::PixelAccessor> createDepthPixelAccessor(RenderingContext & context, Texture& texture);

//! Create a special pixel accessor for reading stencil values. This has to be used for packed depth and stencil image formats.
Util::Reference<Util::PixelAccessor> createStencilPixelAccessor(RenderingContext & context, Texture& texture);

/**
	* Compares two depth texture and determines their minimal distance.
	* Note: This method is heavily customized to support the snapping functionality implemented by getNodeToSceneDistance in scripts/Util/Picking_Utils.escipt.
	* For the comparison the second texture is flipped horizontally and it's values are inverted.
	* In order to have a sound definition of a (positive) distance the implementation checks that the first texture lies completely behind the second one.
	* If this is not the case -1.0f is returned.
	* In case the two textures are disjoint (they don't have a common pixel with a depth value unequal to the clearDepth-value) the method returns -2.0f.
	*/
float minDepthDistance(RenderingContext & context, Texture& firstTex, Texture& secondTex);

}

}

#endif // RENDERING_TEXTUREUTILS_H

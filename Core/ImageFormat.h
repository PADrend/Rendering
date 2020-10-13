/*
	This file is part of the Rendering library.
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_CORE_IMAGEFORMAT_H_
#define RENDERING_CORE_IMAGEFORMAT_H_

#include <cstdint>

#include <Geometry/Vec3.h>

namespace Rendering {

enum class PixelFormat : std::uint8_t {
	Unknown,
	R8Unorm,
	R8Snorm,
	R16Unorm,
	R16Snorm,
	RG8Unorm,
	RG8Snorm,
	RG16Unorm,
	RG16Snorm,
	RGB16Unorm,
	RGB16Snorm,
	RGB5A1Unorm,
	RGBA8Unorm,
	RGBA8Snorm,
	RGB10A2Unorm,
	RGB10A2Uint,
	RGBA16Unorm,
	RGBA8UnormSrgb,
	R16Float,
	RG16Float,
	RGB16Float,
	RGBA16Float,
	R32Float,
	RG32Float,
	RGB32Float,
	RGBA32Float,
	R11G11B10Float,
	RGB9E5Float,
	R8Int,
	R8Uint,
	R16Int,
	R16Uint,
	R32Int,
	R32Uint,
	RG8Int,
	RG8Uint,
	RG16Int,
	RG16Uint,
	RG32Int,
	RG32Uint,
	RGB16Int,
	RGB16Uint,
	RGB32Int,
	RGB32Uint,
	RGBA8Int,
	RGBA8Uint,
	RGBA16Int,
	RGBA16Uint,
	RGBA32Int,
	RGBA32Uint,

	BGRA8Unorm,
	BGRA8UnormSrgb,
	
	R5G6B5Unorm,

	// Depth-stencil
	D32Float,
	D16Unorm,
	D32FloatS8X24,
	D24UnormS8,

	// Compressed formats
	BC1Unorm,   // DXT1
	BC1UnormSrgb, 
	BC2Unorm,   // DXT3
	BC2UnormSrgb,
	BC3Unorm,   // DXT5
	BC3UnormSrgb,
	BC4Unorm,   // RGTC Unsigned Red
	BC4Snorm,   // RGTC Signed Red
	BC5Unorm,   // RGTC Unsigned RG
	BC5Snorm,   // RGTC Signed RG
	BC6HS16,
	BC6HU16,
	BC7Unorm,
	BC7UnormSrgb,
};

struct ImageFormat {
	Geometry::Vec3i extent;
	PixelFormat pixelFormat = PixelFormat::RGBA8Unorm;
	uint32_t mipLevels = 0;
	uint32_t layers = 0;
	uint32_t samples = 1;
};

} /* Rendering */

#endif /* end of include guard: RENDERING_CORE_IMAGEFORMAT_H_ */
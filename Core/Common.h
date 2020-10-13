/*
  This file is part of the Rendering library.
  Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>
  
  This library is subject to the terms of the Mozilla Public License, v. 2.0.
  You should have received a copy of the MPL along with this library; see the
  file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef RENDERING_CORE_COMMON_H_
#define RENDERING_CORE_COMMON_H_

#include "ApiHandles.h"
#include <Geometry/Vec3.h>
#include <Util/Utils.h>

namespace Geometry {
using Vec3ui = _Vec3<uint32_t>;
} /* Geometry */

namespace Rendering {

enum MemoryUsage {
	Unknown, //! No intended memory usage specified.
	CpuOnly, //! Memory will be mappable on host.
	GpuOnly, //! Memory will be used on device only.
	CpuToGpu, //! Memory that is both mappable on host and preferably fast to access by GPU.
	GpuToCpu //! Memory mappable on host and cached.
};

//---------------------------

enum class InternalFormat : std::uint8_t {
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

//---------------------------

struct ImageFormat {
	Geometry::Vec3ui extent;
	InternalFormat pixelFormat = InternalFormat::RGBA8Unorm;
	uint32_t mipLevels = 1;
	uint32_t layers = 1;
	uint32_t samples = 1;
};

//---------------------------

uint32_t convertToApiFormat(const InternalFormat& format);

//-------------

} /* Rendering */

//-------------

template <> struct std::hash<Rendering::ImageFormat> {
	std::size_t operator()(const Rendering::ImageFormat &format) const {
		std::size_t result = 0;
		Util::hash_combine(result, format.extent.getX());
		Util::hash_combine(result, format.extent.getY());
		Util::hash_combine(result, format.extent.getZ());
		Util::hash_combine(result, format.pixelFormat);
		Util::hash_combine(result, format.mipLevels);
		Util::hash_combine(result, format.layers);
		Util::hash_combine(result, format.samples);
		return result;
	}
};

//-------------

#endif /* end of include guard: RENDERING_CORE_COMMON_H_ */

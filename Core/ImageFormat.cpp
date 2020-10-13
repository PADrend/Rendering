/*
	This file is part of the Rendering library.
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "ImageFormat.h"

#include <vulkan/vulkan.hpp>

namespace Rendering {

uint32_t convertToInternalFormat(const PixelFormat& format) {
	switch(format) {
		case PixelFormat::R8Unorm: return VK_FORMAT_R8_UNORM;
		case PixelFormat::R8Snorm: return VK_FORMAT_R8_SNORM;
		case PixelFormat::R16Unorm: return VK_FORMAT_R16_UNORM;
		case PixelFormat::R16Snorm: return VK_FORMAT_R16_SNORM;
		case PixelFormat::RG8Unorm: return VK_FORMAT_R8G8_UNORM;
		case PixelFormat::RG8Snorm: return VK_FORMAT_R8G8_SNORM;
		case PixelFormat::RG16Unorm: return VK_FORMAT_R16G16_UNORM;
		case PixelFormat::RG16Snorm: return VK_FORMAT_R16G16_SNORM;
		case PixelFormat::RGB16Unorm: return VK_FORMAT_R16G16B16_UNORM;
		case PixelFormat::RGB16Snorm: return VK_FORMAT_R16G16B16_SNORM;
		case PixelFormat::RGB5A1Unorm: return VK_FORMAT_A1R5G5B5_UNORM_PACK16;
		case PixelFormat::RGBA8Unorm: return VK_FORMAT_R8G8B8A8_UNORM;
		case PixelFormat::RGBA8Snorm: return VK_FORMAT_R8G8B8A8_SNORM;
		case PixelFormat::RGB10A2Unorm: return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
		case PixelFormat::RGB10A2Uint: return VK_FORMAT_A2R10G10B10_UINT_PACK32;
		case PixelFormat::RGBA16Unorm: return VK_FORMAT_R16G16B16A16_UNORM;
		case PixelFormat::RGBA8UnormSrgb: return VK_FORMAT_R8G8B8A8_SRGB;
		case PixelFormat::R16Float: return VK_FORMAT_R16_SFLOAT;
		case PixelFormat::RG16Float: return VK_FORMAT_R16G16_SFLOAT;
		case PixelFormat::RGB16Float: return VK_FORMAT_R16G16B16_SFLOAT;
		case PixelFormat::RGBA16Float: return VK_FORMAT_R16G16B16A16_SFLOAT;
		case PixelFormat::R32Float: return VK_FORMAT_R32_SFLOAT;
		case PixelFormat::RG32Float: return VK_FORMAT_R32G32_SFLOAT;
		case PixelFormat::RGB32Float: return VK_FORMAT_R32G32B32_SFLOAT;
		case PixelFormat::RGBA32Float: return VK_FORMAT_R32G32B32A32_SFLOAT;
		case PixelFormat::R11G11B10Float: return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
		case PixelFormat::RGB9E5Float: return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;
		case PixelFormat::R8Int: return VK_FORMAT_R8_SINT;
		case PixelFormat::R8Uint: return VK_FORMAT_R8_UINT;
		case PixelFormat::R16Int: return VK_FORMAT_R16_SINT;
		case PixelFormat::R16Uint: return VK_FORMAT_R16_UINT;
		case PixelFormat::R32Int: return VK_FORMAT_R32_SINT;
		case PixelFormat::R32Uint: return VK_FORMAT_R32_UINT;
		case PixelFormat::RG8Int: return VK_FORMAT_R8G8_SINT;
		case PixelFormat::RG8Uint: return VK_FORMAT_R8G8_UINT;
		case PixelFormat::RG16Int: return VK_FORMAT_R16G16_SINT;
		case PixelFormat::RG16Uint: return VK_FORMAT_R16G16_UINT;
		case PixelFormat::RG32Int: return VK_FORMAT_R32G32_SINT;
		case PixelFormat::RG32Uint: return VK_FORMAT_R32G32_UINT;
		case PixelFormat::RGB16Int: return VK_FORMAT_R16G16B16_SINT;
		case PixelFormat::RGB16Uint: return VK_FORMAT_R16G16B16_UINT;
		case PixelFormat::RGB32Int: return VK_FORMAT_R32G32B32_SINT;
		case PixelFormat::RGB32Uint: return VK_FORMAT_R32G32B32_UINT;
		case PixelFormat::RGBA8Int: return VK_FORMAT_R8G8B8A8_SINT;
		case PixelFormat::RGBA8Uint: return VK_FORMAT_R8G8B8A8_UINT;
		case PixelFormat::RGBA16Int: return VK_FORMAT_R16G16B16A16_SINT;
		case PixelFormat::RGBA16Uint: return VK_FORMAT_R16G16B16A16_UINT;
		case PixelFormat::RGBA32Int: return VK_FORMAT_R32G32B32A32_SINT;
		case PixelFormat::RGBA32Uint: return VK_FORMAT_R32G32B32A32_UINT;
		case PixelFormat::BGRA8Unorm: return VK_FORMAT_B8G8R8A8_UNORM;
		case PixelFormat::BGRA8UnormSrgb: return VK_FORMAT_B8G8R8A8_SRGB;
		case PixelFormat::R5G6B5Unorm: return VK_FORMAT_R5G6B5_UNORM_PACK16;
		case PixelFormat::D32Float: return VK_FORMAT_D32_SFLOAT;
		case PixelFormat::D16Unorm: return VK_FORMAT_D16_UNORM;
		case PixelFormat::D32FloatS8X24: return VK_FORMAT_D32_SFLOAT_S8_UINT;
		case PixelFormat::D24UnormS8: return VK_FORMAT_D24_UNORM_S8_UINT;
		case PixelFormat::BC1Unorm: return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
		case PixelFormat::BC1UnormSrgb: return VK_FORMAT_BC1_RGB_SRGB_BLOCK;
		case PixelFormat::BC2Unorm: return VK_FORMAT_BC2_UNORM_BLOCK;
		case PixelFormat::BC2UnormSrgb: return VK_FORMAT_BC2_SRGB_BLOCK;
		case PixelFormat::BC3Unorm: return VK_FORMAT_BC3_UNORM_BLOCK;
		case PixelFormat::BC3UnormSrgb: return VK_FORMAT_BC3_SRGB_BLOCK;
		case PixelFormat::BC4Unorm: return VK_FORMAT_BC4_UNORM_BLOCK;
		case PixelFormat::BC4Snorm: return VK_FORMAT_BC4_SNORM_BLOCK;
		case PixelFormat::BC5Unorm: return VK_FORMAT_BC5_UNORM_BLOCK;
		case PixelFormat::BC5Snorm: return VK_FORMAT_BC5_SNORM_BLOCK;
		case PixelFormat::BC6HS16: return VK_FORMAT_BC6H_SFLOAT_BLOCK;
		case PixelFormat::BC6HU16: return VK_FORMAT_BC6H_UFLOAT_BLOCK;
		case PixelFormat::BC7Unorm: return VK_FORMAT_BC7_UNORM_BLOCK;
		case PixelFormat::BC7UnormSrgb: return VK_FORMAT_BC7_SRGB_BLOCK;
		default: return VK_FORMAT_UNDEFINED;
	}
}

} /* Rendering */
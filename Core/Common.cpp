/*
	This file is part of the Rendering library.
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "Common.h"

#include <vulkan/vulkan.hpp>

namespace Rendering {

uint32_t convertToApiFormat(const InternalFormat& format) {
	switch(format) {
		case InternalFormat::R8Unorm: return VK_FORMAT_R8_UNORM;
		case InternalFormat::R8Snorm: return VK_FORMAT_R8_SNORM;
		case InternalFormat::R16Unorm: return VK_FORMAT_R16_UNORM;
		case InternalFormat::R16Snorm: return VK_FORMAT_R16_SNORM;
		case InternalFormat::RG8Unorm: return VK_FORMAT_R8G8_UNORM;
		case InternalFormat::RG8Snorm: return VK_FORMAT_R8G8_SNORM;
		case InternalFormat::RG16Unorm: return VK_FORMAT_R16G16_UNORM;
		case InternalFormat::RG16Snorm: return VK_FORMAT_R16G16_SNORM;
		case InternalFormat::RGB16Unorm: return VK_FORMAT_R16G16B16_UNORM;
		case InternalFormat::RGB16Snorm: return VK_FORMAT_R16G16B16_SNORM;
		case InternalFormat::RGB5A1Unorm: return VK_FORMAT_A1R5G5B5_UNORM_PACK16;
		case InternalFormat::RGBA8Unorm: return VK_FORMAT_R8G8B8A8_UNORM;
		case InternalFormat::RGBA8Snorm: return VK_FORMAT_R8G8B8A8_SNORM;
		case InternalFormat::RGB10A2Unorm: return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
		case InternalFormat::RGB10A2Uint: return VK_FORMAT_A2R10G10B10_UINT_PACK32;
		case InternalFormat::RGBA16Unorm: return VK_FORMAT_R16G16B16A16_UNORM;
		case InternalFormat::RGBA8UnormSrgb: return VK_FORMAT_R8G8B8A8_SRGB;
		case InternalFormat::R16Float: return VK_FORMAT_R16_SFLOAT;
		case InternalFormat::RG16Float: return VK_FORMAT_R16G16_SFLOAT;
		case InternalFormat::RGB16Float: return VK_FORMAT_R16G16B16_SFLOAT;
		case InternalFormat::RGBA16Float: return VK_FORMAT_R16G16B16A16_SFLOAT;
		case InternalFormat::R32Float: return VK_FORMAT_R32_SFLOAT;
		case InternalFormat::RG32Float: return VK_FORMAT_R32G32_SFLOAT;
		case InternalFormat::RGB32Float: return VK_FORMAT_R32G32B32_SFLOAT;
		case InternalFormat::RGBA32Float: return VK_FORMAT_R32G32B32A32_SFLOAT;
		case InternalFormat::R11G11B10Float: return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
		case InternalFormat::RGB9E5Float: return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;
		case InternalFormat::R8Int: return VK_FORMAT_R8_SINT;
		case InternalFormat::R8Uint: return VK_FORMAT_R8_UINT;
		case InternalFormat::R16Int: return VK_FORMAT_R16_SINT;
		case InternalFormat::R16Uint: return VK_FORMAT_R16_UINT;
		case InternalFormat::R32Int: return VK_FORMAT_R32_SINT;
		case InternalFormat::R32Uint: return VK_FORMAT_R32_UINT;
		case InternalFormat::RG8Int: return VK_FORMAT_R8G8_SINT;
		case InternalFormat::RG8Uint: return VK_FORMAT_R8G8_UINT;
		case InternalFormat::RG16Int: return VK_FORMAT_R16G16_SINT;
		case InternalFormat::RG16Uint: return VK_FORMAT_R16G16_UINT;
		case InternalFormat::RG32Int: return VK_FORMAT_R32G32_SINT;
		case InternalFormat::RG32Uint: return VK_FORMAT_R32G32_UINT;
		case InternalFormat::RGB16Int: return VK_FORMAT_R16G16B16_SINT;
		case InternalFormat::RGB16Uint: return VK_FORMAT_R16G16B16_UINT;
		case InternalFormat::RGB32Int: return VK_FORMAT_R32G32B32_SINT;
		case InternalFormat::RGB32Uint: return VK_FORMAT_R32G32B32_UINT;
		case InternalFormat::RGBA8Int: return VK_FORMAT_R8G8B8A8_SINT;
		case InternalFormat::RGBA8Uint: return VK_FORMAT_R8G8B8A8_UINT;
		case InternalFormat::RGBA16Int: return VK_FORMAT_R16G16B16A16_SINT;
		case InternalFormat::RGBA16Uint: return VK_FORMAT_R16G16B16A16_UINT;
		case InternalFormat::RGBA32Int: return VK_FORMAT_R32G32B32A32_SINT;
		case InternalFormat::RGBA32Uint: return VK_FORMAT_R32G32B32A32_UINT;
		case InternalFormat::BGRA8Unorm: return VK_FORMAT_B8G8R8A8_UNORM;
		case InternalFormat::BGRA8UnormSrgb: return VK_FORMAT_B8G8R8A8_SRGB;
		case InternalFormat::R5G6B5Unorm: return VK_FORMAT_R5G6B5_UNORM_PACK16;
		case InternalFormat::D32Float: return VK_FORMAT_D32_SFLOAT;
		case InternalFormat::D16Unorm: return VK_FORMAT_D16_UNORM;
		case InternalFormat::D32FloatS8X24: return VK_FORMAT_D32_SFLOAT_S8_UINT;
		case InternalFormat::D24UnormS8: return VK_FORMAT_D24_UNORM_S8_UINT;
		case InternalFormat::BC1Unorm: return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
		case InternalFormat::BC1UnormSrgb: return VK_FORMAT_BC1_RGB_SRGB_BLOCK;
		case InternalFormat::BC2Unorm: return VK_FORMAT_BC2_UNORM_BLOCK;
		case InternalFormat::BC2UnormSrgb: return VK_FORMAT_BC2_SRGB_BLOCK;
		case InternalFormat::BC3Unorm: return VK_FORMAT_BC3_UNORM_BLOCK;
		case InternalFormat::BC3UnormSrgb: return VK_FORMAT_BC3_SRGB_BLOCK;
		case InternalFormat::BC4Unorm: return VK_FORMAT_BC4_UNORM_BLOCK;
		case InternalFormat::BC4Snorm: return VK_FORMAT_BC4_SNORM_BLOCK;
		case InternalFormat::BC5Unorm: return VK_FORMAT_BC5_UNORM_BLOCK;
		case InternalFormat::BC5Snorm: return VK_FORMAT_BC5_SNORM_BLOCK;
		case InternalFormat::BC6HS16: return VK_FORMAT_BC6H_SFLOAT_BLOCK;
		case InternalFormat::BC6HU16: return VK_FORMAT_BC6H_UFLOAT_BLOCK;
		case InternalFormat::BC7Unorm: return VK_FORMAT_BC7_UNORM_BLOCK;
		case InternalFormat::BC7UnormSrgb: return VK_FORMAT_BC7_SRGB_BLOCK;
		default: return VK_FORMAT_UNDEFINED;
	}
}

} /* Rendering */
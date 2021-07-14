/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
  Copyright (C) 2021 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifdef RENDERING_HAVE_LIB_DDS

#include "StreamerDDS.h"
#include "../Texture/Texture.h"
#include "../Texture/TextureUtils.h"
#include "../GLHeader.h"
#include <algorithm>
#include <cstdint>

#define DDSKTX_IMPLEMENT
#include <dds-ktx.h>

namespace Rendering {
namespace Serialization {

inline PixelFormatGL ktxToGLFormat(ddsktx_format format) {
	switch(format) {
		case DDSKTX_FORMAT_BC1: return { 0, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT };
		case DDSKTX_FORMAT_BC2: return { 0, 0, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT };
		case DDSKTX_FORMAT_BC3: return { 0, 0, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT };
		case DDSKTX_FORMAT_BC4: return { 0, 0, GL_COMPRESSED_RED_RGTC1 };
		case DDSKTX_FORMAT_BC5: return { 0, 0, GL_COMPRESSED_RG_RGTC2 };
		case DDSKTX_FORMAT_BC6H:  return { 0, 0, GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT };
		case DDSKTX_FORMAT_BC7:  return { 0, 0, GL_COMPRESSED_RGBA_BPTC_UNORM };
		case DDSKTX_FORMAT_ETC1:  return { 0, 0, GL_ETC1_RGB8_OES };
		case DDSKTX_FORMAT_ETC2:  return { 0, 0, GL_COMPRESSED_RGB8_ETC2 };
		case DDSKTX_FORMAT_ETC2A:  return { 0, 0, GL_COMPRESSED_RGBA8_ETC2_EAC };
		case DDSKTX_FORMAT_ETC2A1:  return { 0, 0, GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2 };
		case DDSKTX_FORMAT_PTC12:  return { 0, 0, GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG };
		case DDSKTX_FORMAT_PTC14:  return { 0, 0, GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG };
		case DDSKTX_FORMAT_PTC12A:  return { 0, 0, GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG };
		case DDSKTX_FORMAT_PTC14A:  return { 0, 0, GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG };
		case DDSKTX_FORMAT_PTC22:  return { 0, 0, GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG };
		case DDSKTX_FORMAT_PTC24:  return { 0, 0, GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG };
		case DDSKTX_FORMAT_ATC:  return { 0, 0, GL_ATC_RGB_AMD };
		case DDSKTX_FORMAT_ATCE:  return { 0, 0, GL_ATC_RGBA_EXPLICIT_ALPHA_AMD };
		case DDSKTX_FORMAT_ATCI:  return { 0, 0, GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD };
		case DDSKTX_FORMAT_ASTC4x4:  return { 0, 0, GL_COMPRESSED_RGBA_ASTC_4x4_KHR };
		case DDSKTX_FORMAT_ASTC5x5:  return { 0, 0, GL_COMPRESSED_RGBA_ASTC_5x5_KHR };
		case DDSKTX_FORMAT_ASTC6x6:  return { 0, 0, GL_COMPRESSED_RGBA_ASTC_6x6_KHR };
		case DDSKTX_FORMAT_ASTC8x5:  return { 0, 0, GL_COMPRESSED_RGBA_ASTC_8x5_KHR };
		case DDSKTX_FORMAT_ASTC8x6:  return { 0, 0, GL_COMPRESSED_RGBA_ASTC_8x6_KHR };
		case DDSKTX_FORMAT_ASTC10x5:  return { 0, 0, GL_COMPRESSED_RGBA_ASTC_10x5_KHR };
		case DDSKTX_FORMAT_A8:  return { GL_ALPHA, GL_UNSIGNED_BYTE, GL_ALPHA8 };
		case DDSKTX_FORMAT_R8:  return { GL_RED, GL_UNSIGNED_BYTE, GL_R8 };
		case DDSKTX_FORMAT_RGBA8:  return { GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA8 };
		case DDSKTX_FORMAT_RGBA8S:  return { GL_RGBA, GL_BYTE, GL_RGBA8_SNORM };
		case DDSKTX_FORMAT_RG16:  return { GL_RG, GL_UNSIGNED_SHORT, GL_RG16 };
		case DDSKTX_FORMAT_RGB8:  return { GL_RGB, GL_UNSIGNED_BYTE, GL_RGB8 };
		case DDSKTX_FORMAT_R16:  return { GL_RED, GL_UNSIGNED_SHORT, GL_R16 };
		case DDSKTX_FORMAT_R32F:  return { GL_RED, GL_FLOAT, GL_R32F };
		case DDSKTX_FORMAT_R16F:  return { GL_RED, GL_HALF_FLOAT, GL_R16F };
		case DDSKTX_FORMAT_RG16F:  return { GL_RG, GL_HALF_FLOAT, GL_RG16F };
		case DDSKTX_FORMAT_RG16S:  return { GL_RG, GL_SHORT, GL_RG16_SNORM };
		case DDSKTX_FORMAT_RGBA16F:  return { GL_RGBA, GL_HALF_FLOAT, GL_RGBA16F };
		case DDSKTX_FORMAT_RGBA16:  return { GL_RGBA, GL_UNSIGNED_SHORT, GL_RGBA16 };
		case DDSKTX_FORMAT_BGRA8:  return { GL_BGRA, GL_UNSIGNED_BYTE, GL_BGRA8_EXT };
		case DDSKTX_FORMAT_RGB10A2:  return { GL_RGBA, GL_UNSIGNED_INT_10_10_10_2, GL_RGB10_A2 };
		case DDSKTX_FORMAT_RG11B10F:  return { GL_RGB, GL_UNSIGNED_INT_10F_11F_11F_REV, GL_R11F_G11F_B10F };
		case DDSKTX_FORMAT_RG8:  return { GL_RG, GL_UNSIGNED_BYTE, GL_RG8 };
		case DDSKTX_FORMAT_RG8S:  return { GL_RG, GL_BYTE, GL_RG8_SNORM };
		default: return {};
	}
}

const char * const StreamerDDS::fileExtension = "dds";

Util::Reference<Texture> StreamerDDS::loadTexture(std::istream & input, TextureType type, uint32_t numLayers){
	if(type!=TextureType::TEXTURE_2D || numLayers!=1){
		WARN("StreamerDDS: Only single layered 2d textures are supported!");
		return nullptr;
	}

	input.seekg(0, std::ios::end);
	std::streampos size = input.tellg();
	input.seekg(0, std::ios::beg);

	std::vector<uint8_t> data(size);
	input.read(reinterpret_cast<char *>(data.data()), size);

	ddsktx_texture_info info;
	ddsktx_error err;
	if(!ddsktx_parse(&info, data.data(), static_cast<int>(size), &err)) {
		WARN("Error: " + std::string(err.msg));
		return nullptr;
	}

	ddsktx_sub_data sub_data;
	ddsktx_get_sub(&info, &sub_data, reinterpret_cast<char *>(data.data()), static_cast<int>(size), 0, 0, 0);

	Texture::Format format;
	format.sizeX = info.width;
	format.sizeY = info.height;
	format.numLayers = info.num_layers;
	format.glTextureType = TextureUtils::textureTypeToGLTextureType(type);
	format.pixelFormat = ktxToGLFormat(info.format);
	format.pixelFormat.compressed = true;
	format.compressedImageSize = format.pixelFormat.compressed ? sub_data.size_bytes : 0;

	Util::Reference<Texture> texture = new Texture(format);
	texture->allocateLocalData();
	std::memcpy(texture->getLocalData(), sub_data.buff, sub_data.size_bytes);

	return texture;
}

uint8_t StreamerDDS::queryCapabilities(const std::string & extension) {
	if(extension == fileExtension) {
		return CAP_LOAD_TEXTURE;
	} else {
		return 0;
	}
}

}
}

#endif // RENDERING_HAVE_LIB_DDS
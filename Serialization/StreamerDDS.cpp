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
#include "../Core/Common.h"
#include <algorithm>
#include <cstdint>
#include <cstring>

#define DDSKTX_IMPLEMENT
#include <dds-ktx.h>

namespace Rendering {
namespace Serialization {

inline InternalFormat ktxToGLFormat(ddsktx_format format) {
	switch(format) {
		case DDSKTX_FORMAT_BC1: return InternalFormat::BC1Unorm;
		case DDSKTX_FORMAT_BC2: return InternalFormat::BC2Unorm;
		case DDSKTX_FORMAT_BC3: return InternalFormat::BC3Unorm;
		case DDSKTX_FORMAT_BC4: return InternalFormat::BC4Snorm;
		case DDSKTX_FORMAT_BC5: return InternalFormat::BC5Snorm;
		case DDSKTX_FORMAT_BC6H:  return InternalFormat::BC6HS16;
		case DDSKTX_FORMAT_BC7:  return InternalFormat::BC7Unorm;
		//case DDSKTX_FORMAT_ETC1:  return InternalFormat::ETC1;
		case DDSKTX_FORMAT_ETC2:  return InternalFormat::ETC2RGB8Unorm;
		//case DDSKTX_FORMAT_ETC2A:  return InternalFormat::ETC2A;
		//case DDSKTX_FORMAT_ETC2A1:  return InternalFormat::ETC2A1;
		//case DDSKTX_FORMAT_PTC12:  return InternalFormat::PTC12;
		//case DDSKTX_FORMAT_PTC14:  return InternalFormat::PTC14;
		//case DDSKTX_FORMAT_PTC12A:  return InternalFormat::PTC12A;
		//case DDSKTX_FORMAT_PTC14A:  return InternalFormat::PTC14A;
		//case DDSKTX_FORMAT_PTC22:  return InternalFormat::PTC22;
		//case DDSKTX_FORMAT_PTC24:  return InternalFormat::PTC24;
		//case DDSKTX_FORMAT_ATC:  return InternalFormat::ATC;
		//case DDSKTX_FORMAT_ATCE:  return InternalFormat::ATCE;
		//case DDSKTX_FORMAT_ATCI:  return InternalFormat::ATCI;
		//case DDSKTX_FORMAT_ASTC4x4:  return InternalFormat::ASTC4x4;
		//case DDSKTX_FORMAT_ASTC5x5:  return InternalFormat::ASTC5x5;
		//case DDSKTX_FORMAT_ASTC6x6:  return InternalFormat::ASTC6x6;
		//case DDSKTX_FORMAT_ASTC8x5:  return InternalFormat::ASTC8x5;
		//case DDSKTX_FORMAT_ASTC8x6:  return InternalFormat::ASTC8x6;
		//case DDSKTX_FORMAT_ASTC10x5:  return InternalFormat::ASTC10x5;
		//case DDSKTX_FORMAT_A8:  return InternalFormat::A8;
		//case DDSKTX_FORMAT_R8:  return InternalFormat::R8;
		case DDSKTX_FORMAT_RGBA8:  return InternalFormat::RGBA8Unorm;
		case DDSKTX_FORMAT_RGBA8S:  return InternalFormat::RGBA8Snorm;
		case DDSKTX_FORMAT_RG16:  return InternalFormat::RG16Uint;
		//case DDSKTX_FORMAT_RGB8:  return InternalFormat::RGB8;
		case DDSKTX_FORMAT_R16:  return InternalFormat::R16Int;
		case DDSKTX_FORMAT_R32F:  return InternalFormat::R32Float;
		case DDSKTX_FORMAT_R16F:  return InternalFormat::R16Float;
		case DDSKTX_FORMAT_RG16F:  return InternalFormat::RG16Float;
		case DDSKTX_FORMAT_RG16S:  return InternalFormat::RG16Snorm;
		case DDSKTX_FORMAT_RGBA16F:  return InternalFormat::RGBA16Float;
		case DDSKTX_FORMAT_RGBA16:  return InternalFormat::RGBA16Int;
		case DDSKTX_FORMAT_BGRA8:  return InternalFormat::BGRA8Unorm;
		case DDSKTX_FORMAT_RGB10A2:  return InternalFormat::RGB10A2Uint;
		case DDSKTX_FORMAT_RG11B10F:  return InternalFormat::R11G11B10Float;
		case DDSKTX_FORMAT_RG8:  return InternalFormat::RG8Int;
		case DDSKTX_FORMAT_RG8S:  return InternalFormat::RG8Snorm;
		default: return InternalFormat::Unknown;
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
	format.extent.setValue(info.width, info.height, info.depth);
	format.layers = info.num_layers;
	format.mipLevels = info.num_mips;
	format.pixelFormat = ktxToGLFormat(info.format);
	format.samples = 1;

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
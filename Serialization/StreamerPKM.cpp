/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "StreamerPKM.h"
#include "../Texture/Texture.h"
#include <algorithm>
#include <cstdint>

namespace Rendering {
namespace Serialization {

const char * const StreamerPKM::fileExtension = "pkm";

inline static uint16_t convertBigEndianTwoBytes(const uint8_t bytes[2]) {
	uint16_t number = 0;
	number |= bytes[0];
	number <<= 8;
	number |= bytes[1];
	return number;
}

Util::Reference<Texture> StreamerPKM::loadTexture(std::istream & input, TextureType type, uint32_t numLayers){
	if(type!=TextureType::TEXTURE_2D || numLayers!=1){
		WARN("StreamerPKM: Only single layered 2d textures are supported!");
		return nullptr;
	}
	
	struct PKMHeader {
		uint8_t magic[4];
		uint8_t version[2];
		uint8_t textureType[2];
		uint8_t width[2];
		uint8_t height[2];
		uint8_t activeWidth[2];
		uint8_t activeHeight[2];
	};

	PKMHeader header;
	input.read(reinterpret_cast<char *>(&header), sizeof(PKMHeader));

	if(!std::equal(header.magic, header.magic + 4, "PKM ")) {
		WARN("Invalid magic found in PKM header.");
		return nullptr;
	}
	if(!std::equal(header.version, header.version + 2, "10")) {
		WARN("Invalid version found in PKM header.");
		return nullptr;
	}
	if(header.textureType[0] != 0 || header.textureType[1] != 0) {
		WARN("Unsupported texture type found in PKM header.");
		return nullptr;
	}
	// Numbers are stored big-endian.
	const uint16_t width = convertBigEndianTwoBytes(header.width);
	const uint16_t height = convertBigEndianTwoBytes(header.height);
	const uint16_t activeWidth = convertBigEndianTwoBytes(header.activeWidth);
	const uint16_t activeHeight = convertBigEndianTwoBytes(header.activeHeight);


	Texture::Format format;
	format.extent = {activeWidth, activeHeight, 1};
	format.pixelFormat = InternalFormat::ETC2RGB8Unorm;
	size_t compressedImageSize = 8 * ((width + 3) >> 2) * ((height + 3) >> 2);

	Util::Reference<Texture> texture = new Texture(format);
	texture->allocateLocalData();
	input.read(reinterpret_cast<char *>(texture->getLocalData()), compressedImageSize);

	return texture;
}

uint8_t StreamerPKM::queryCapabilities(const std::string & extension) {
	if(extension == fileExtension) {
		return CAP_LOAD_TEXTURE;
	} else {
		return 0;
	}
}

}
}

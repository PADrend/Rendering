/*
	This file is part of the Rendering library.
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "TextureType.h"


namespace Rendering {

std::string getTypeString(const TextureType& type) {
	switch(type) {
		case TextureType::TEXTURE_1D: return "TEXTURE_1D";
		case TextureType::TEXTURE_1D_ARRAY: return "TEXTURE_1D_ARRAY";
		case TextureType::TEXTURE_2D: return "TEXTURE_2D";
		case TextureType::TEXTURE_2D_ARRAY: return "TEXTURE_2D_ARRAY";
		case TextureType::TEXTURE_3D: return "TEXTURE_3D";
		case TextureType::TEXTURE_CUBE_MAP: return "TEXTURE_CUBE_MAP";
		case TextureType::TEXTURE_CUBE_MAP_ARRAY: return "TEXTURE_CUBE_MAP_ARRAY";
		case TextureType::TEXTURE_BUFFER: return "TEXTURE_BUFFER";
		case TextureType::TEXTURE_2D_MULTISAMPLE: return "TEXTURE_2D_MULTISAMPLE";
	}
}

} /* Rendering */
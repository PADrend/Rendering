/*
	This file is part of the Rendering library.
	Copyright (C) 2014 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef TEXTURE_TYPE_H
#define TEXTURE_TYPE_H

#include <cstdint>
#include <string>

namespace Rendering {

/*! A Texture's type. (Corresponds to 'glTextureType', but the actual value is independent from OpenGL.
	\note Value assignment must never change! (they may be used for serialization)
	@ingroup texture
*/
enum class TextureType : std::uint8_t{
	TEXTURE_1D = 0,
	TEXTURE_1D_ARRAY = 1,
	TEXTURE_2D = 2,
	TEXTURE_2D_ARRAY = 3,
	TEXTURE_3D = 4,
	TEXTURE_CUBE_MAP = 5,
	TEXTURE_CUBE_MAP_ARRAY = 6,
	TEXTURE_BUFFER = 7,
	TEXTURE_2D_MULTISAMPLE = 8
};

std::string getTypeString(const TextureType& type);

}

#endif // TEXTURE_TYPE_H

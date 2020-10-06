/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef VERTEXATTRIBUTECONSTS_H
#define VERTEXATTRIBUTECONSTS_H

#include <Util/StringIdentifier.h>
#include <cstdint>

namespace Rendering {
	
//! @ingroup mesh
namespace VertexAttributeIds{

RENDERINGAPI extern const Util::StringIdentifier 	POSITION;
RENDERINGAPI extern const Util::StringIdentifier 	NORMAL;
RENDERINGAPI extern const Util::StringIdentifier 	COLOR;
RENDERINGAPI extern const Util::StringIdentifier 	TANGENT;
RENDERINGAPI extern const Util::StringIdentifier 	TEXCOORD0;
RENDERINGAPI extern const Util::StringIdentifier 	TEXCOORD1;
RENDERINGAPI extern const Util::StringIdentifier 	TEXCOORD2;
RENDERINGAPI extern const Util::StringIdentifier 	TEXCOORD3;
RENDERINGAPI extern const Util::StringIdentifier 	TEXCOORD4;
RENDERINGAPI extern const Util::StringIdentifier 	TEXCOORD5;
RENDERINGAPI extern const Util::StringIdentifier 	TEXCOORD6;
RENDERINGAPI extern const Util::StringIdentifier 	TEXCOORD7;

//! Helper function to access the texture coordinate identifiers TEXCOORD0 to TEXCOORD7 by a variable.
RENDERINGAPI extern Util::StringIdentifier getTextureCoordinateIdentifier(uint_fast8_t textureUnit);

}

// ----------------------------------
}
#endif // VERTEXATTRIBUTECONSTS_H

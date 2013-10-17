/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "VertexAttributeIds.h"
#include <stdexcept>

namespace Rendering {

const Util::StringIdentifier 	VertexAttributeIds::POSITION("sg_Position");
const Util::StringIdentifier 	VertexAttributeIds::NORMAL("sg_Normal");
const Util::StringIdentifier 	VertexAttributeIds::COLOR("sg_Color");
const Util::StringIdentifier 	VertexAttributeIds::TANGENT("sg_Tangent");
const Util::StringIdentifier 	VertexAttributeIds::TEXCOORD0("sg_TexCoord0");
const Util::StringIdentifier 	VertexAttributeIds::TEXCOORD1("sg_TexCoord1");
const Util::StringIdentifier 	VertexAttributeIds::TEXCOORD2("sg_TexCoord2");
const Util::StringIdentifier 	VertexAttributeIds::TEXCOORD3("sg_TexCoord3");
const Util::StringIdentifier 	VertexAttributeIds::TEXCOORD4("sg_TexCoord4");
const Util::StringIdentifier 	VertexAttributeIds::TEXCOORD5("sg_TexCoord5");
const Util::StringIdentifier 	VertexAttributeIds::TEXCOORD6("sg_TexCoord6");
const Util::StringIdentifier 	VertexAttributeIds::TEXCOORD7("sg_TexCoord7");

//! (static)
Util::StringIdentifier VertexAttributeIds::getTextureCoordinateIdentifier(uint_fast8_t textureUnit) {
	switch(textureUnit) {
		case 0:
			return TEXCOORD0;
		case 1:
			return TEXCOORD1;
		case 2:
			return TEXCOORD2;
		case 3:
			return TEXCOORD3;
		case 4:
			return TEXCOORD4;
		case 5:
			return TEXCOORD5;
		case 6:
			return TEXCOORD6;
		case 7:
			return TEXCOORD7;
		default:
			throw std::out_of_range("At most eight texture coordinate attributes are supported.");
	}
}

}

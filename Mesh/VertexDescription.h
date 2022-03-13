/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	Copyright (C) 2018-2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef VERTEXDESCRIPTION_H
#define VERTEXDESCRIPTION_H

#include "VertexAttribute.h"
#include "VertexAttributeIds.h"
#include "../Helper.h"

#include <Util/Resources/ResourceFormat.h>

#include <cstddef>
#include <deque>
#include <cstdint>
#include <string>

namespace Util {
class StringIdentifier;
}
namespace Rendering {

/**
 * VertexDescription
 * @ingroup mesh
 */
class VertexDescription : public Util::ResourceFormat {
	public:

		//! Add an attribute with the given name and the given number of float values.
		const VertexAttribute & appendFloatAttribute(const Util::StringIdentifier & nameId, uint32_t numValues) { 
			return appendFloat(nameId, numValues);
		}

		//! Add an attribute with the given name and the given number of unsigned int values.
		const VertexAttribute & appendUnsignedIntAttribute(const Util::StringIdentifier & nameId, uint32_t numValues, bool convertToFloat=false) {
			return appendUInt(nameId, numValues);
		}

		//! Add an RGBA color attribute. It is stored as four unsigned byte values.
		const VertexAttribute & appendColorRGBAByte() {
			return appendAttribute(VertexAttributeIds::COLOR, Util::TypeConstant::UINT8, 4, true);
		}

		//! Add an RGB color attribute. It is stored as three float values.
		const VertexAttribute & appendColorRGBFloat() {
			return appendAttribute(VertexAttributeIds::COLOR, Util::TypeConstant::FLOAT, 3, false);
		}

		//! Add an RGBA color attribute. It is stored as four float values.
		const VertexAttribute & appendColorRGBAFloat() {
			return appendAttribute(VertexAttributeIds::COLOR, Util::TypeConstant::FLOAT, 4, false);
		}

		//! Add a three-dimensional normal attribute. It is stored as four byte values.
		const VertexAttribute & appendNormalByte() {
			return appendAttribute(VertexAttributeIds::NORMAL, Util::TypeConstant::INT8, 4, true);
		}

		//! Add a three-dimensional normal attribute. It is stored as three float values.
		const VertexAttribute & appendNormalFloat() {
			return appendAttribute(VertexAttributeIds::NORMAL, Util::TypeConstant::FLOAT, 3, false);
		}

		//! Add a two-dimensional position attribute. It is stored as two float values.
		const VertexAttribute & appendPosition2D() {
			return appendAttribute(VertexAttributeIds::POSITION, Util::TypeConstant::FLOAT, 2, false);
		}

		//! Add a three-dimensional position attribute. It is stored as three float values.
		const VertexAttribute & appendPosition3D() {
			return appendAttribute(VertexAttributeIds::POSITION, Util::TypeConstant::FLOAT, 3, false);
		}

		//! Add a three-dimensional position attribute. It is stored as four float values.
		const VertexAttribute & appendPosition4D() {
			return appendAttribute(VertexAttributeIds::POSITION, Util::TypeConstant::FLOAT, 4, false);
		}
		
		//! Add a three-dimensional position attribute. It is stored as four half float values.
		const VertexAttribute & appendPosition4DHalf() {
			return appendAttribute(VertexAttributeIds::POSITION, Util::TypeConstant::HALF, 4, false);
		}

		//! Add a texture coordinate attribute. It is stored as two float values.
		const VertexAttribute & appendTexCoord(uint_fast8_t textureUnit = 0) {
			return appendAttribute(VertexAttributeIds::getTextureCoordinateIdentifier(textureUnit), Util::TypeConstant::FLOAT, 2, false);
		}

		uint64_t getVertexSize() const { return getSize(); }
};
// ----------------------------------
}
#endif // VERTEXDESCRIPTION_H

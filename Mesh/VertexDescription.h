/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef VERTEXDESCRIPTION_H
#define VERTEXDESCRIPTION_H

#include "VertexAttribute.h"
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
class VertexDescription {
	public:

		/*! \note This MUST NOT be of type std::vector, as it has to be assured that the Attributes
				are not re-located in memory when appending additional attributes. (references may be stored) */
		typedef std::deque<VertexAttribute> attributeContainer_t;

		RENDERINGAPI VertexDescription();

		/*! Create and add a new attribute to the vertexDescription.
			\return the new attribute
			\note the owner of the attribute is the vertexDescription
			\note Before using this function, check a default method can be used instead (e.g. append appendPosition3D) */
		RENDERINGAPI const VertexAttribute & appendAttribute(const Util::StringIdentifier & nameId, uint8_t numValues, uint32_t glType, bool normalize, bool convertToFloat=true);
		RENDERINGAPI const VertexAttribute & appendAttribute(const Util::StringIdentifier & nameId, uint8_t numValues, uint32_t glType);
		RENDERINGAPI const VertexAttribute & appendAttribute(const std::string & name, uint8_t numValues, uint32_t glType, bool normalize, bool convertToFloat=true);

		//! Add an attribute with the given name and the given number of float values.
		RENDERINGAPI const VertexAttribute & appendFloatAttribute(const Util::StringIdentifier & nameId, uint8_t numValues);

		//! Add an attribute with the given name and the given number of unsigned int values.
		RENDERINGAPI const VertexAttribute & appendUnsignedIntAttribute(const Util::StringIdentifier & nameId, uint8_t numValues, bool convertToFloat=true);

		//! Add an RGBA color attribute. It is stored as four unsigned byte values.
		RENDERINGAPI const VertexAttribute & appendColorRGBAByte();

		//! Add an RGB color attribute. It is stored as three float values.
		RENDERINGAPI const VertexAttribute & appendColorRGBFloat();

		//! Add an RGBA color attribute. It is stored as four float values.
		RENDERINGAPI const VertexAttribute & appendColorRGBAFloat();

		//! Add a three-dimensional normal attribute. It is stored as four byte values.
		RENDERINGAPI const VertexAttribute & appendNormalByte();

		//! Add a three-dimensional normal attribute. It is stored as three float values.
		RENDERINGAPI const VertexAttribute & appendNormalFloat();

		//! Add a two-dimensional position attribute. It is stored as two float values.
		RENDERINGAPI const VertexAttribute & appendPosition2D();

		//! Add a three-dimensional position attribute. It is stored as three float values.
		RENDERINGAPI const VertexAttribute & appendPosition3D();

		//! Add a three-dimensional position attribute. It is stored as four float values.
		RENDERINGAPI const VertexAttribute & appendPosition4D();
		
		//! Add a three-dimensional position attribute. It is stored as four half float values.
		RENDERINGAPI const VertexAttribute & appendPosition4DHalf();

		//! Add a texture coordinate attribute. It is stored as two float values.
		RENDERINGAPI const VertexAttribute & appendTexCoord(uint_fast8_t textureUnit = 0);

		/*! Get a reference to the attribute with the corresponding name.
			\return Always returns an attribute.
					If the attribute is not present in the vertex description, it is empty.
			\note The owner of the attribute is the vertexDescription, so be careful if the
					vertexDescription is deleted or reassigned.*/
		RENDERINGAPI const VertexAttribute & getAttribute(const Util::StringIdentifier & nameId)const;
		RENDERINGAPI const VertexAttribute & getAttribute(const std::string & name)const;

		RENDERINGAPI bool hasAttribute(const Util::StringIdentifier & nameId)const;
		RENDERINGAPI bool hasAttribute(const std::string & name)const;

		/**
		 * Update an existing attribute of or append a new attribute to the VertexDescription.
		 *
		 * @param attr Attribute that contains the new data.
		 * @note The offsets of all attributes may be recalculated and therefore old values may become invalid.
		 */
		RENDERINGAPI void updateAttribute(const VertexAttribute & attr);

		size_t getVertexSize()const							{	return vertexSize;	}
		size_t getNumAttributes()const						{	return attributes.size();	}
		const attributeContainer_t & getAttributes()const	{	return attributes;	}
		RENDERINGAPI bool operator==(const VertexDescription & other)const;
		RENDERINGAPI bool operator<(const VertexDescription & other)const;

		RENDERINGAPI std::string toString()const;

	private:
		attributeContainer_t attributes;
		size_t vertexSize;

};
// ----------------------------------
}
#endif // VERTEXDESCRIPTION_H

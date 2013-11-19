/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef VERTEXATTRIBUTE_H_
#define VERTEXATTRIBUTE_H_

#include <Util/StringIdentifier.h>
#include <cstdint>
#include <string>

namespace Rendering {

/**
 * Description of a single attribute of a vertex.
 * For example it can describe a three-dimensional position (three float values) or an RGBA color (four unsigned bytes).
 *
 * @author Claudius Jähn, Benjamin Eikel
 * @date 2011-07-21
 */
class VertexAttribute {
	public:
		VertexAttribute();
		VertexAttribute(uint8_t _numValues, uint32_t _dataType, Util::StringIdentifier _nameId);
		bool operator==(const VertexAttribute & other)const{
			return nameId==other.nameId && offset==other.offset && numValues==other.numValues && dataType==other.dataType;
		}
		bool operator<(const VertexAttribute & other)const;
		std::string toString()const;

		bool empty()const					{	return numValues==0;	}
		uint16_t getOffset()const			{	return offset;	}
		uint16_t getDataSize()const			{	return dataSize;	}
		uint8_t getNumValues()const			{	return numValues;	}
		uint32_t getDataType()const			{	return dataType;	}
		Util::StringIdentifier getNameId()const	{	return nameId;	}
		const std::string & getName()const	{	return name;	}
	private:
		VertexAttribute(uint16_t _offset,uint8_t _numValues, uint32_t _dataType, Util::StringIdentifier _nameId,std::string  _name);
		friend class VertexDescription;
		uint16_t offset;
		uint16_t dataSize;
		uint8_t numValues;
		uint32_t dataType;
		Util::StringIdentifier nameId;
		std::string name;
};

}

#endif /* VERTEXATTRIBUTE_H_ */

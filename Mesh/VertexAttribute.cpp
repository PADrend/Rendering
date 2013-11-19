/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "VertexAttribute.h"
#include "../GLHeader.h"
#include "../Helper.h"
#include <Util/Macros.h>
#include <Util/StringIdentifier.h>
#include <sstream>

namespace Rendering {

VertexAttribute::VertexAttribute() :
		offset(0),dataSize(0),
		numValues(0), dataType(GL_FLOAT), nameId(0){
}

//! (ctor)
VertexAttribute::VertexAttribute(uint8_t _numValues, uint32_t _dataType, Util::StringIdentifier _nameId) :
		offset(0), dataSize(getGLTypeSize(_dataType) * _numValues),
		numValues(_numValues), dataType(_dataType), nameId(std::move(_nameId)), name() {
	if((dataSize % 4) != 0) {
		WARN("VertexAttribute is not 4-byte aligned.");
	}
}

//! (ctor)
VertexAttribute::VertexAttribute(uint16_t _offset,uint8_t _numValues, uint32_t _dataType, Util::StringIdentifier _nameId,std::string  _name) :
		offset(_offset),dataSize(getGLTypeSize(_dataType)*_numValues),
		numValues(_numValues), dataType(_dataType), nameId(std::move(_nameId)),name(std::move(_name)){
	if((dataSize % 4) != 0) {
		WARN("VertexAttribute is not 4-byte aligned.");
	}
}

bool VertexAttribute::operator<(const VertexAttribute & other)const{
	// \note name and dataSize need not to be compared
	if(numValues!=other.numValues){
		return numValues<other.numValues;
	}else if(dataType!=other.dataType){
		return dataType<other.dataType;
	}else if(nameId!=other.nameId){
		return nameId<other.nameId;
	}else if(offset!=other.offset){
		return offset<other.offset;
	} else return false;
}

std::string VertexAttribute::toString()const{
	std::ostringstream s;
	s << name << ": " << static_cast<unsigned int>(numValues) << " " << getGLTypeString(dataType);
	return s.str();
}

}

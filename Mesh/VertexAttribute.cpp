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
		numValues(0), dataType(GL_FLOAT), nameId(0), normalize(false), convertToFloat(true){
}

//! (ctor)
VertexAttribute::VertexAttribute(uint8_t _numValues, uint32_t _dataType, Util::StringIdentifier _nameId, bool _normalize, bool _convertToFloat /*= true*/) :
		offset(0), dataSize(getGLTypeSize(_dataType) * _numValues),
		numValues(_numValues), dataType(_dataType), nameId(std::move(_nameId)), name(), normalize(_normalize), convertToFloat(_convertToFloat) {
	if((dataSize % 4) != 0) {
		WARN("VertexAttribute is not 4-byte aligned.");
	}
	if(!convertToFloat) {
		switch(dataType) {
			case GL_BYTE:
			case GL_UNSIGNED_BYTE:
			case GL_SHORT:
			case GL_UNSIGNED_SHORT:
			case GL_INT:
			case GL_UNSIGNED_INT:
				break;
			default:
				WARN("VertexAttribute with convertToFloat=false is only allowed for GL_BYTE, GL_UNSIGNED_BYTE, GL_SHORT, GL_UNSIGNED_SHORT, GL_INT, GL_UNSIGNED_INT");
		}
	}
}

//! (ctor)
VertexAttribute::VertexAttribute(uint16_t _offset,uint8_t _numValues, uint32_t _dataType, Util::StringIdentifier _nameId,std::string _name, bool _normalize, bool _convertToFloat /*= true*/) :
		offset(_offset),dataSize(getGLTypeSize(_dataType)*_numValues),
		numValues(_numValues), dataType(_dataType), nameId(std::move(_nameId)),name(std::move(_name)), normalize(_normalize), convertToFloat(_convertToFloat){
	if((dataSize % 4) != 0) {
		WARN("VertexAttribute is not 4-byte aligned.");
	}
	if(!convertToFloat) {
		switch(dataType) {
			case GL_BYTE:
			case GL_UNSIGNED_BYTE:
			case GL_SHORT:
			case GL_UNSIGNED_SHORT:
			case GL_INT:
			case GL_UNSIGNED_INT:
				break;
			default:
				WARN("VertexAttribute with convertToFloat=false is only allowed for GL_BYTE, GL_UNSIGNED_BYTE, GL_SHORT, GL_UNSIGNED_SHORT, GL_INT, GL_UNSIGNED_INT");
		}
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
	}else if(normalize!=other.normalize){
		return normalize<other.normalize;
	}else if(convertToFloat!=other.convertToFloat){
		return convertToFloat<other.convertToFloat;
	} else return false;
}

std::string VertexAttribute::toString()const{
	std::ostringstream s;
	s << name << ": " << static_cast<unsigned int>(numValues) << " " << getGLTypeString(dataType);
	if(normalize)
		s << " (normalize)";
	return s.str();
}

}

/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "VertexDescription.h"
#include "VertexAttributeIds.h"
#include "../GLHeader.h"
#include <Util/StringIdentifier.h>
#include <iterator>
#include <sstream>

namespace Rendering {


//! (ctor)
VertexDescription::VertexDescription() : attributes(), vertexSize(0) {
}

const VertexAttribute & VertexDescription::appendAttribute(const Util::StringIdentifier & nameId, uint8_t numValues, uint32_t type) {
	attributes.push_back(VertexAttribute(getVertexSize(), numValues, type, nameId, nameId.toString()));
	vertexSize += attributes.back().getDataSize();
	return attributes.back();
}

const VertexAttribute & VertexDescription::appendAttribute(const std::string & name, uint8_t numValues, uint32_t type) {
	attributes.push_back(VertexAttribute(getVertexSize(), numValues, type, Util::StringIdentifier(name), name));
	vertexSize += attributes.back().getDataSize();
	return attributes.back();
}

const VertexAttribute & VertexDescription::appendFloatAttribute(const Util::StringIdentifier & nameId, uint8_t numValues) {
	return appendAttribute(nameId, numValues, GL_FLOAT);
}

const VertexAttribute & VertexDescription::appendUnsignedIntAttribute(const Util::StringIdentifier & nameId, uint8_t numValues) {
	return appendAttribute(nameId, numValues, GL_UNSIGNED_INT);
}

const VertexAttribute & VertexDescription::getAttribute(const std::string & name) const {
	return getAttribute(Util::StringIdentifier(name));
}

const VertexAttribute & VertexDescription::getAttribute(const Util::StringIdentifier & nameId) const {
	static const VertexAttribute emptyAttribute;
	for(const auto & attr : getAttributes()) {
		if(attr.getNameId() == nameId) {
			return attr;
		}
	}
	return emptyAttribute;
}

bool VertexDescription::hasAttribute(const std::string & name) const {
	return hasAttribute(Util::StringIdentifier(name));
}

bool VertexDescription::hasAttribute(const Util::StringIdentifier & nameId) const {
	for(const auto & attr : getAttributes()) {
		if(attr.getNameId() == nameId) {
			return true;
		}
	}
	return false;
}

void VertexDescription::updateAttribute(const VertexAttribute & attr) {
	for(auto it = attributes.begin(); it != attributes.end(); ++it) {
		VertexAttribute & currentAttr = *it;
		if(currentAttr.getNameId() == attr.getNameId()) {
			currentAttr = VertexAttribute(currentAttr.getOffset(), attr.getNumValues(), attr.getDataType(), currentAttr.getNameId(), currentAttr.getName());
			// Update the offsets.
			vertexSize = static_cast<std::size_t>(currentAttr.getOffset() + currentAttr.getDataSize());

			auto toUpdateIt = it;
			std::advance(toUpdateIt, 1);
			for(; toUpdateIt != attributes.end(); ++toUpdateIt) {
				VertexAttribute & toUpdateAttr = *toUpdateIt;
				toUpdateAttr.offset = vertexSize;
				vertexSize += toUpdateAttr.getDataSize();
			}
			return;
		}
	}
	// Attribute was not found.
	appendAttribute(attr.getNameId(), attr.getNumValues(), attr.getDataType());
}

std::string VertexDescription::toString() const {
	std::ostringstream s;
	s << "(VertexDescription";
	for(const auto & attr : getAttributes()) {
		s << ", " << attr.toString();
	}
	s<< ")";
	return s.str();
}

bool VertexDescription::operator==(const VertexDescription & other) const {
	return getVertexSize() == other.getVertexSize() &&
		   getAttributes() == other.getAttributes();
}

bool VertexDescription::operator<(const VertexDescription & other)const{
	if(getVertexSize() != other.getVertexSize()){
		return getVertexSize() < other.getVertexSize();
	}else if(getNumAttributes() != other.getNumAttributes()){
		return getNumAttributes() < other.getNumAttributes();
	}
	auto it1=getAttributes().begin();
	for(auto it2=other.getAttributes().begin() ; it2!=other.getAttributes().end() ; ++it1,++it2){
		if( ! ((*it1)==(*it2)) ){
			return (*it1)<(*it2);
		}
	}
	return false;
}


const VertexAttribute & VertexDescription::appendColorRGBAByte() {
	return appendAttribute(VertexAttributeIds::COLOR, 4, GL_UNSIGNED_BYTE);
}

const VertexAttribute & VertexDescription::appendColorRGBFloat() {
	return appendAttribute(VertexAttributeIds::COLOR, 3, GL_FLOAT);
}

const VertexAttribute & VertexDescription::appendColorRGBAFloat() {
	return appendAttribute(VertexAttributeIds::COLOR, 4, GL_FLOAT);
}

const VertexAttribute & VertexDescription::appendNormalByte() {
	return appendAttribute(VertexAttributeIds::NORMAL, 4, GL_BYTE);
}

const VertexAttribute & VertexDescription::appendNormalFloat() {
	return appendAttribute(VertexAttributeIds::NORMAL, 3, GL_FLOAT);
}

const VertexAttribute & VertexDescription::appendPosition2D() {
	return appendAttribute(VertexAttributeIds::POSITION, 2, GL_FLOAT);
}

const VertexAttribute & VertexDescription::appendPosition3D() {
	return appendAttribute(VertexAttributeIds::POSITION, 3, GL_FLOAT);
}

const VertexAttribute & VertexDescription::appendTexCoord() {
	return appendAttribute(VertexAttributeIds::TEXCOORD0, 2, GL_FLOAT);
}


}

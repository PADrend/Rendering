/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	Copyright (C) 2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "MeshIndexData.h"
#include "../GLHeader.h"
#include "../Helper.h"
#include "../RenderingContext/RenderingContext.h"
#include <Util/Macros.h>
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>
#include <iostream>

namespace Rendering {

/*! (ctor)  */
MeshIndexData::MeshIndexData() : BufferView(nullptr, 0, sizeof(uint32_t)),
			minIndex(0), maxIndex(0), dataChanged(false) {
}

/*! (ctor)  */
MeshIndexData::MeshIndexData(const MeshIndexData & other) : BufferView(other),
			minIndex(other.getMinIndex()), maxIndex(other.getMaxIndex()), dataChanged(true) {
	if(other.hasLocalData()) {
		indexArray = other.indexArray;
	} else if(other.isUploaded()) {
		other.downloadTo(indexArray);
	} else {
		WARN("Cannot access index data."); // should not happen
	}
}

//!(internal)
void MeshIndexData::releaseLocalData(){
	indexArray.clear();
	indexArray.shrink_to_fit();
}

void MeshIndexData::swap(MeshIndexData & other){
	if(this == &other)
		return;
		
	BufferView::swap(other);
	std::swap(minIndex, other.minIndex);
	std::swap(maxIndex, other.maxIndex);
	std::swap(dataChanged, other.dataChanged);
	std::swap(indexArray, other.indexArray);
}

void MeshIndexData::allocate(uint32_t count) {
	setElementCount(count);
	indexArray.resize(count, std::numeric_limits<uint32_t>::max());
	markAsChanged();
}

void MeshIndexData::updateIndexRange() {
	if(indexArray.empty()) {
		minIndex = 1;
		maxIndex = 0;
	} else {
		auto minMaxPair = std::minmax_element(indexArray.begin(), indexArray.end());
		minIndex = *minMaxPair.first;
		maxIndex = *minMaxPair.second;
	}
}

bool MeshIndexData::upload() {
	return upload(0);
}

//!	(internal)
bool MeshIndexData::upload(uint32_t flags){
	if(getElementCount() == 0 || indexArray.empty() )
		return false;

	try {
		if(!isValid() || (getOffset() + getBuffer()->getSize()) < dataSize()) {
			removeGlBuffer();
			allocateBuffer(flags, reinterpret_cast<const uint8_t*>(indexArray.data()));
		} else {
			getBuffer()->upload(reinterpret_cast<const uint8_t*>(indexArray.data()), getOffset());
		}
		GET_GL_ERROR()
	}
	catch (...) {
		WARN("VBO: upload failed");
		removeGlBuffer();
		return false;
	}
	dataChanged = false;
	return true;
}

//!	(internal)
bool MeshIndexData::download(){
	if(!isValid() || getElementCount()==0)
		return false;
	downloadTo(indexArray);
	dataChanged = false;
	return true;
}

//!	(internal)
void MeshIndexData::downloadTo(std::vector<uint32_t> & destination) const {
	destination = getValues<uint32_t>(0U, getElementCount());
}

//!	(internal)
void MeshIndexData::removeGlBuffer(){
	setBuffer(nullptr);
}

}

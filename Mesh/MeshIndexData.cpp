/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "MeshIndexData.h"
#include "../Core/Device.h"
#include "../Core/BufferStorage.h"
#include <Util/Macros.h>
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace Rendering {

//-----------------

/*! (ctor)  */
MeshIndexData::MeshIndexData() : MeshIndexData(Device::getDefault()) { }

//-----------------

/*! (ctor)  */
MeshIndexData::MeshIndexData(const DeviceRef& device) :
			indexCount(0), minIndex(0), maxIndex(0),
			bufferObject(BufferObject::create(device)), dataChanged(false) {
}

//-----------------


/*! (ctor)  */
MeshIndexData::MeshIndexData(const MeshIndexData & other) :
			indexCount(other.getIndexCount()), 
			minIndex(other.getMinIndex()), maxIndex(other.getMaxIndex()),
			bufferObject(), dataChanged(true) {
	if(other.hasLocalData()) {
		indexArray = other.indexArray;
	} else if(other.isUploaded()) {
		other.downloadTo(indexArray);
	} else {
		WARN("Cannot access index data."); // should not happen
	}
}

//-----------------

MeshIndexData::MeshIndexData(MeshIndexData &&) = default;

//-----------------

MeshIndexData::~MeshIndexData() = default;

//-----------------

//!(internal)
void MeshIndexData::releaseLocalData(){
	indexArray.clear();
	indexArray.shrink_to_fit();
}

//-----------------

void MeshIndexData::swap(MeshIndexData & other){
	if(this == &other)
		return;

	using std::swap;
	swap(indexCount, other.indexCount);
	swap(minIndex, other.minIndex);
	swap(maxIndex, other.maxIndex);
	swap(bufferObject, other.bufferObject);
	swap(dataChanged, other.dataChanged);
	swap(indexArray, other.indexArray);
}

//-----------------

void MeshIndexData::allocate(uint32_t count) {
	indexCount = count;
	indexArray.resize(indexCount, std::numeric_limits<uint32_t>::max());
	indexArray.shrink_to_fit();
	markAsChanged();
}

//-----------------

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

//-----------------

bool MeshIndexData::upload(MemoryUsage usage) {
	if(indexCount == 0 || indexArray.empty() )
		return false;
	
	size_t size = indexCount * sizeof(uint32_t);
	if(!bufferObject || !bufferObject->isValid() || bufferObject->getSize() != size || bufferObject->getBuffer()->getConfig().access != usage) {
		// Allocate new buffer
		bufferObject->allocate(indexArray.size(), ResourceUsage::IndexBuffer, usage);
	}
	// TODO: copy data if only usage has changed
	bufferObject->upload(indexArray);
	dataChanged = false;
	return true;
}

//-----------------

//!	(internal)
bool MeshIndexData::download(){
	if(!isUploaded() || indexCount==0)
		return false;
	downloadTo(indexArray);
	dataChanged = false;
	return true;
}

//-----------------

//!	(internal)
void MeshIndexData::downloadTo(std::vector<uint32_t> & destination) const {
	destination = bufferObject->download<uint32_t>(getIndexCount());
}

//-----------------

/*! (internal) */
void MeshIndexData::drawElements(bool useVBO,uint32_t drawMode,uint32_t startIndex,uint32_t numberOfIndices){
	if(startIndex+numberOfIndices>getIndexCount())
		throw std::out_of_range("MeshIndexData::drawElements: Accessing invalid index.");
	
}

//-----------------

}

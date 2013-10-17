/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "MeshIndexData.h"
#include "../GLHeader.h"
#include "../Helper.h"
#include <Util/Macros.h>
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace Rendering {

/*! (ctor)  */
MeshIndexData::MeshIndexData() :
			indexCount(0), minIndex(0), maxIndex(0),
			bufferObject(),  dataChanged(false) {
}

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

//!(internal)
void MeshIndexData::releaseLocalData(){
	indexArray.clear();
	indexArray.shrink_to_fit();
}

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

void MeshIndexData::allocate(uint32_t count) {
	indexCount = count;
	indexArray.resize(indexCount, std::numeric_limits<uint32_t>::max());
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
	return upload(GL_STATIC_DRAW);
}

//!	(internal)
bool MeshIndexData::upload(uint32_t usageHint){
	if( isUploaded() )
		removeGlBuffer();

	if(indexCount == 0 || indexArray.empty() )
		return false;

	try {
		bufferObject.uploadData(GL_ELEMENT_ARRAY_BUFFER, indexArray, usageHint);
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
	if(!isUploaded() || indexCount==0)
		return false;
	downloadTo(indexArray);
	dataChanged = false;
	return true;
}

//!	(internal)
#ifdef LIB_GL
void MeshIndexData::downloadTo(std::vector<uint32_t> & destination) const {
	destination = bufferObject.downloadData<uint32_t>(GL_ELEMENT_ARRAY_BUFFER, getIndexCount());
}
#else
void MeshIndexData::downloadTo(std::vector<uint32_t> & /*destination*/) const {
	WARN("downloadTo not supported.");
}
#endif

//!	(internal)
void MeshIndexData::removeGlBuffer(){
	bufferObject.destroy();
}

/*! (internal) */
void MeshIndexData::drawElements(bool useVBO,uint32_t drawMode,uint32_t startIndex,uint32_t numberOfIndices){
	if(startIndex+numberOfIndices>getIndexCount())
		throw std::out_of_range("MeshIndexData::drawElements: Accessing invalid index.");
	
#ifdef LIB_GL
	if(useVBO && isUploaded()) { // VBO
		bufferObject.bind(GL_ELEMENT_ARRAY_BUFFER);
		glDrawRangeElements(drawMode, getMinIndex(), getMaxIndex(), numberOfIndices, GL_UNSIGNED_INT, reinterpret_cast<void*>(sizeof(GLuint)*startIndex));
		bufferObject.unbind(GL_ELEMENT_ARRAY_BUFFER);
	} else if(hasLocalData()) { // VertexArray
		glDrawRangeElements(drawMode, getMinIndex(), getMaxIndex(), numberOfIndices, GL_UNSIGNED_INT,  reinterpret_cast<void*>(data()+startIndex));
	}
#else
	if (useVBO && isUploaded()) { // VBO
		bufferObject.bind(GL_ELEMENT_ARRAY_BUFFER);
		glDrawElements(drawMode, numberOfIndices, GL_UNSIGNED_INT, reinterpret_cast<void*>(sizeof(GLuint)*startIndex));
		bufferObject.unbind(GL_ELEMENT_ARRAY_BUFFER);
	} else if (hasLocalData()) { // VertexArray
		glDrawElements(drawMode, numberOfIndices, GL_UNSIGNED_INT, reinterpret_cast<void*>(data()+startIndex));
	}
#endif
}

}

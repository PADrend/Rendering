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
#include "MeshVertexData.h"
#include "VertexAttributeIds.h"
#include "VertexDescription.h"
#include "VertexAttributeAccessors.h"
#include "../Core/Device.h"
#include "../Core/BufferStorage.h"
#include "../Shader/Shader.h"
#include "../RenderingContext/RenderingContext.h"
#include <Util/Macros.h>
#include <algorithm>
#include <cstdint>
#include <memory>
#include <mutex>
#include <set>
#include <vector>
#include <utility>

namespace Rendering {

//-----------------

//! (internal)
void MeshVertexData::setVertexDescription(const VertexDescription & vd) {
	static std::mutex mutex;
	std::lock_guard<std::mutex> lock(mutex);

	static std::set<VertexDescription> descriptionCollections;
	std::pair<std::set<VertexDescription>::iterator,bool> result = descriptionCollections.insert(vd);
	vertexDescription = &*(result.first);
}

//-----------------

//! (ctor)
MeshVertexData::MeshVertexData() : MeshVertexData(Device::getDefault()) { }

//-----------------

MeshVertexData::MeshVertexData(const DeviceRef& device) :
	device(device), binaryData(), vertexDescription(nullptr), vertexCount(0), bufferObject(), bb(), dataChanged(false) {
	setVertexDescription(VertexDescription());
}

//-----------------

//! (ctor)
MeshVertexData::MeshVertexData(const MeshVertexData & other) :
	device(other.device), binaryData(), vertexDescription(other.vertexDescription), vertexCount(other.getVertexCount()), 
	bufferObject(), bb(other.getBoundingBox()), dataChanged(true) {
	if(other.hasLocalData()) {
		binaryData = other.binaryData;
	} else if(other.isUploaded()) {
		other.downloadTo(binaryData);
	} else {
		WARN("Cannot access vertex data."); // should not happen
	}
}

//-----------------

MeshVertexData::MeshVertexData(MeshVertexData &&) = default;

//-----------------

MeshVertexData::~MeshVertexData() = default;

//-----------------

void MeshVertexData::releaseLocalData() {
	binaryData.resize(0);
	binaryData.shrink_to_fit();
}

//-----------------

void MeshVertexData::swap(MeshVertexData & other) {
	if(this == &other)
		return;

	using std::swap;
	swap(vertexDescription, other.vertexDescription);
	swap(vertexCount, other.vertexCount);
	swap(bufferObject, other.bufferObject);
	swap(bb, other.bb);
	swap(dataChanged, other.dataChanged);
	swap(binaryData, other.binaryData);
}

//-----------------

void MeshVertexData::allocate(uint32_t count, const VertexDescription & vd) {
	setVertexDescription(vd);
	vertexCount = count;
	binaryData.resize(vd.getVertexSize() * count);
	binaryData.shrink_to_fit();
	markAsChanged();
}

//-----------------

const uint8_t * MeshVertexData::operator[](uint32_t index) const {
	return binaryData.data() + index * vertexDescription->getVertexSize();

}

//-----------------

uint8_t * MeshVertexData::operator[](uint32_t index) {
	return binaryData.data() + index * vertexDescription->getVertexSize();
}

//-----------------

void MeshVertexData::updateBoundingBox() {
	if (vertexCount == 0) {
		bb = Geometry::Box();
		return;
	}
	const VertexDescription & vd=getVertexDescription();	
	auto acc = FloatAttributeAccessor::create(*this, VertexAttributeIds::POSITION);
	
	const VertexAttribute & attr = vd.getAttribute(VertexAttributeIds::POSITION);
	const uint8_t vertexNum = attr.getNumValues();
	if (vertexNum < 1) {
		WARN(std::string("Vertex component count is zero."));
		return;
	}
	
	// The following implementation calculates minima and maxima for the coordinates.
	// This is faster than calling Geometry::Box::include for each vertex.
	std::vector<float> min(vertexNum, std::numeric_limits<float>::max());
	std::vector<float> max(vertexNum, std::numeric_limits<float>::lowest());
	for (uint_fast32_t i = 0; i < vertexCount; ++i) {
		auto p = acc->getValues(i);
		for (uint_fast8_t dim = 0; dim < vertexNum; ++dim) {
			if (p[dim] < min[dim]) {
				min[dim] = p[dim];
			}
			if (p[dim] > max[dim]) {
				max[dim] = p[dim];
			}
		}
	}

	if (vertexNum == 1) {
		bb = Geometry::Box(min[0], max[0], 0.0f, 0.0f, 0.0f, 0.0f);
	} else if (vertexNum == 2) {
		bb = Geometry::Box(min[0], max[0], min[1], max[1], 0.0f, 0.0f);
	} else {
		bb = Geometry::Box(min[0], max[0], min[1], max[1], min[2], max[2]);
	}
}

//-----------------

bool MeshVertexData::upload(MemoryUsage usage) {
	if(vertexCount == 0 || binaryData.empty() )
		return false;
	
	if(!bufferObject || !bufferObject->isValid() || bufferObject->getSize() != binaryData.size() || bufferObject->getBuffer()->getConfig().access != usage) {
		// Allocate new buffer
		bufferObject->allocate(binaryData.size(), ResourceUsage::VertexBuffer, usage);
	}
	// TODO: copy data if only usage has changed
	bufferObject->upload(binaryData);
	dataChanged = false;
	return true;
}

//-----------------

bool MeshVertexData::download() {
	if(!isUploaded() || vertexCount==0)
		return false;
	downloadTo(binaryData);
	dataChanged = false;
	return true;
}

//-----------------

void MeshVertexData::downloadTo(std::vector<uint8_t> & destination) const {
	const std::size_t numBytes = getVertexDescription().getVertexSize() * getVertexCount();
	destination = bufferObject->download(numBytes);
}

//-----------------

void MeshVertexData::bind(RenderingContext & context, bool useVBO) {
	const VertexDescription & vd = getVertexDescription();
	if(dataChanged)
		upload();
	
	/*

	Shader * shader = context.getActiveShader();
	const GLsizei vSize=vd.getVertexSize();

	if (shader != nullptr && shader->usesSGUniforms()) {
		for(const auto & attr : vd.getAttributes()) {
			if(!attr.empty()) {
				context.enableVertexAttribArray(attr, vertexPosition, vSize);
			}
		}
	}*/

}

//-----------------

/*! (internal) */
void MeshVertexData::drawArray(RenderingContext & context,bool useVBO,uint32_t drawMode,uint32_t startIndex,uint32_t numberOfElements) {
	if(startIndex+numberOfElements>getVertexCount())
		throw std::out_of_range("MeshIndexData::drawElements: Accessing invalid index.");
	
	//bind(context,useVBO);
	//glDrawArrays(drawMode, startIndex, numberOfElements);
	//unbind(context,useVBO);
}

//-----------------

void MeshVertexData::unbind(RenderingContext & context, bool useVBO) {
}

//-----------------

}

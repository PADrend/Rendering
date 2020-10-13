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
#include "Mesh.h"
#include "MeshDataStrategy.h"
#include "VertexDescription.h"
#include "../RenderingContext.h"
#include <Util/IO/FileName.h>
#include <Util/ReferenceCounter.h>
#include <Util/Macros.h>
#include <algorithm>
#include <utility>

namespace Rendering {

	//-----------------

Mesh::Mesh() :
		ReferenceCounter_t(), fileName(), dataStrategy(MeshDataStrategy::getDefaultStrategy()),topology(PrimitiveTopology::TriangleList), useIndexData(true) {
}

//-----------------

Mesh::Mesh(MeshIndexData meshIndexData, MeshVertexData meshVertexData) :
		ReferenceCounter_t(), indexData(std::move(meshIndexData)), fileName(), vertexData(std::move(meshVertexData)), 
		dataStrategy(MeshDataStrategy::getDefaultStrategy()), topology(PrimitiveTopology::TriangleList), useIndexData(true) {
}

//-----------------

Mesh::Mesh(const VertexDescription & desc, uint32_t vertexCount, uint32_t indexCount) :
		ReferenceCounter_t(), fileName(), dataStrategy(MeshDataStrategy::getDefaultStrategy()), topology(PrimitiveTopology::TriangleList), useIndexData(true) {
	indexData.allocate(indexCount);
	vertexData.allocate(vertexCount, desc);
}

//-----------------

Mesh::~Mesh() = default;

//-----------------

Mesh * Mesh::clone()const{
	return new Mesh(*this);
}

//-----------------

void Mesh::swap(Mesh & m){
	_getIndexData().swap(m._getIndexData());
	_getVertexData().swap(m._getVertexData());

	using std::swap;
	swap(dataStrategy, m.dataStrategy);
	swap(fileName, m.fileName);
	swap(topology, m.topology);
	swap(useIndexData, m.useIndexData);
}

//-----------------

size_t Mesh::getMainMemoryUsage() const {
	return sizeof(Mesh) + indexData.dataSize() + vertexData.dataSize();
}

//-----------------

size_t Mesh::getGraphicsMemoryUsage() const {
	return (indexData.isUploaded() ? indexData.getIndexCount() * sizeof(uint32_t) : 0)
			+ (vertexData.isUploaded() ? vertexData.getVertexCount() * vertexData.getVertexDescription().getVertexSize() : 0);
}

//-----------------

void Mesh::_display(RenderingContext & context,uint32_t firstElement,uint32_t elementCount) {
	dataStrategy->prepare(this);
	dataStrategy->displayMesh(context, this,firstElement,elementCount);
}

//-----------------

uint32_t Mesh::getPrimitiveCount(uint32_t numElements) const {
	if(numElements == 0) {
		numElements = (useIndexData ? indexData.getIndexCount() : getVertexCount());
	}
	switch(topology) {
		case PrimitiveTopology::LineList: return numElements / 2;
		case PrimitiveTopology::LineStrip: return numElements - 1;
		case PrimitiveTopology::TriangleList: return numElements / 3;
		case PrimitiveTopology::TriangleStrip:
		case PrimitiveTopology::TriangleFan: return numElements - 2;
		case PrimitiveTopology::LineListWithAdjacency: return numElements / 4;
		case PrimitiveTopology::LineStripWithAdjacency: return numElements - 3;
		case PrimitiveTopology::TriangleListWithAdjacency: return numElements / 6;
		case PrimitiveTopology::TriangleStripWithAdjacency: return (numElements - 4)/2;
		default: return numElements;
	}
}

//-----------------

MeshIndexData & Mesh::openIndexData(){
	dataStrategy->assureLocalIndexData(this);
	return indexData;
}

//-----------------

MeshVertexData & Mesh::openVertexData(){
	dataStrategy->assureLocalVertexData(this);
	return vertexData;
}

//-----------------

void Mesh::setDataStrategy(MeshDataStrategy * newStrategy) {
	dataStrategy = newStrategy;
}

//-----------------

Mesh::draw_mode_t Mesh::getDrawMode() const {
	switch(topology) {
		case PrimitiveTopology::PointList: return DRAW_POINTS;
		case PrimitiveTopology::LineList: return DRAW_LINES;
		case PrimitiveTopology::LineStrip: return DRAW_LINE_STRIP;
		case PrimitiveTopology::TriangleList: return DRAW_TRIANGLES;
		default:
		WARN("Mesh: Unsupported topology.");
		return DRAW_TRIANGLES; 
	}
}

//-----------------

void Mesh::setDrawMode(draw_mode_t newMode) {	
	switch(newMode) {
		case DRAW_POINTS: topology = PrimitiveTopology::PointList; break;
		case DRAW_LINE_STRIP: topology = PrimitiveTopology::LineStrip; break;
		case DRAW_LINE_LOOP:
			WARN("Mesh: Unsupported topology: DRAW_LINE_LOOP.");
			topology = PrimitiveTopology::LineStrip;
			break;
		case DRAW_LINES: topology = PrimitiveTopology::LineList; break;
		case DRAW_TRIANGLES: topology = PrimitiveTopology::TriangleList; break;
		default: break;
	}
}

//-----------------
}

/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "Mesh.h"
#include "MeshDataStrategy.h"
#include "VertexDescription.h"
#include "../RenderingContext/RenderingContext.h"
#include "../GLHeader.h"
#include <Util/IO/FileName.h>
#include <Util/ReferenceCounter.h>
#include <algorithm>
#include <utility>

namespace Rendering {

// if this assertions ever fails, the data type for Mesh::triangleMode should be changed to uint32_t
static_assert(GL_TRIANGLES<256 && GL_LINES<256 && GL_POINTS<256 && GL_LINE_STRIP<256 && GL_LINE_LOOP<256,
			"Constants for Mesh's triangleMode are expected to fit into a single byte; This should be true on all platforms."); 

Mesh::Mesh() :
		ReferenceCounter_t(), fileName(), dataStrategy(MeshDataStrategy::getDefaultStrategy()),drawMode(DRAW_TRIANGLES), useIndexData(true) {
}

Mesh::Mesh(const MeshIndexData & meshIndexData, const MeshVertexData & meshVertexData) :
		ReferenceCounter_t(), indexData(meshIndexData), fileName(), vertexData(meshVertexData), 
		dataStrategy(MeshDataStrategy::getDefaultStrategy()), drawMode(DRAW_TRIANGLES), useIndexData(true) {
}

Mesh::Mesh(MeshIndexData && meshIndexData, MeshVertexData && meshVertexData) :
		ReferenceCounter_t(), indexData(std::move(meshIndexData)), fileName(), vertexData(std::move(meshVertexData)), 
		dataStrategy(MeshDataStrategy::getDefaultStrategy()), drawMode(DRAW_TRIANGLES), useIndexData(true) {
}

Mesh::Mesh(const VertexDescription & desc, uint32_t vertexCount, uint32_t indexCount) :
		ReferenceCounter_t(), fileName(), dataStrategy(MeshDataStrategy::getDefaultStrategy()), drawMode(DRAW_TRIANGLES), useIndexData(true) {
	indexData.allocate(indexCount);
	vertexData.allocate(vertexCount, desc);
}

Mesh * Mesh::clone()const{
	return new Mesh(*this);
}

void Mesh::swap(Mesh & m){
	_getIndexData().swap(m._getIndexData());
	_getVertexData().swap(m._getVertexData());

	using std::swap;
	swap(dataStrategy, m.dataStrategy);
	swap(fileName, m.fileName);
	swap(drawMode, m.drawMode);
	swap(useIndexData, m.useIndexData);
}

size_t Mesh::getMainMemoryUsage() const {
	return sizeof(Mesh) + indexData.dataSize() + vertexData.dataSize();
}

size_t Mesh::getGraphicsMemoryUsage() const {
	return 	(indexData.isUploaded() ? indexData.getIndexCount() * sizeof(uint32_t) : 0)
			+ (vertexData.isUploaded() ? vertexData.getVertexCount() * vertexData.getVertexDescription().getVertexSize() : 0);
}

void Mesh::_display(RenderingContext & context,uint32_t firstElement,uint32_t elementCount) {
	context.applyChanges();
	dataStrategy->prepare(this);
	dataStrategy->displayMesh(context, this,firstElement,elementCount);
}

uint32_t Mesh::getPrimitiveCount(uint32_t numElements) const {
	if(numElements == 0) {
		numElements = (useIndexData ? indexData.getIndexCount() : getVertexCount());
	}
	switch(drawMode) {
		case DRAW_LINE_STRIP:
			return numElements - 1;
		case DRAW_LINES:
			return numElements / 2;
		case DRAW_TRIANGLES:
			return numElements / 3;
		case DRAW_POINTS:
		case DRAW_LINE_LOOP:
		default:
			break;
	}
	return numElements;
}

MeshIndexData & Mesh::openIndexData(){
	dataStrategy->assureLocalIndexData(this);
	return indexData;
}

MeshVertexData & Mesh::openVertexData(){
	dataStrategy->assureLocalVertexData(this);
	return vertexData;
}

void Mesh::setDataStrategy(MeshDataStrategy * newStrategy) {
	dataStrategy = newStrategy;
}

uint32_t Mesh::getGLDrawMode() const {
	switch(drawMode) {
		case DRAW_POINTS:
			return GL_POINTS;
		case DRAW_LINE_STRIP:
			return GL_LINE_STRIP;
		case DRAW_LINE_LOOP:
			return GL_LINE_LOOP;
		case DRAW_LINES:
			return GL_LINES;
		case DRAW_TRIANGLES:
			return GL_TRIANGLES;
		default:
			break;
	}
	return GL_INVALID_ENUM;
}

void Mesh::setGLDrawMode(uint32_t glDrawMode) {
	if(glDrawMode == GL_POINTS) {
		drawMode = DRAW_POINTS;
	} else if(glDrawMode == GL_LINE_STRIP) {
		drawMode = DRAW_LINE_STRIP;
	} else if(glDrawMode == GL_LINE_LOOP) {
		drawMode = DRAW_LINE_LOOP;
	} else if(glDrawMode == GL_LINES) {
		drawMode = DRAW_LINES;
	} else {
		drawMode = DRAW_TRIANGLES;
	}
}

}

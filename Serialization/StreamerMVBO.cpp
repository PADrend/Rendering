/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "StreamerMVBO.h"
#include "Serialization.h"
#include "../Mesh/Mesh.h"
#include "../Mesh/VertexAttributeIds.h"
#include "../Mesh/VertexDescription.h"
#include <Util/GenericAttribute.h>

/// \todo Show compile error when using a machine without LITTLE-ENDIANness

using namespace Util;
namespace Rendering {

const char * const StreamerMVBO::fileExtension = "mvbo";

Mesh * StreamerMVBO::loadMesh(std::istream & input){
	uint32_t numVertices = read(input);
	uint32_t numIndices = read(input)*3;

	VertexDescription vd;
	vd.appendPosition3D();
	const VertexAttribute & normalAttr = vd.appendNormalByte();
	const VertexAttribute & colorAttr = vd.appendColorRGBAByte();

	auto mesh = new Mesh;
	MeshVertexData & vData=mesh->openVertexData();
	vData.allocate(numVertices,vd);

	// read vertex data
	size_t vertexDataSize = vData.dataSize();// numVertices * 5 * sizeof(uint32_t);
	uint8_t * vertexData = vData.data();
	input.read(reinterpret_cast<char *> (vertexData), vertexDataSize);
	// switch position of entries
	for(uint8_t * cursor = vertexData;cursor<vertexData+vertexDataSize;cursor+=5 * sizeof(uint32_t)){
		uint32_t color=*(reinterpret_cast<uint32_t *>(cursor+12));
		uint32_t normals=*(reinterpret_cast<uint32_t *>(cursor+16));
		*(reinterpret_cast<uint32_t *>(cursor+colorAttr.getOffset()))=color;
		*(reinterpret_cast<uint32_t *>(cursor+normalAttr.getOffset()))=normals;
	}
	vData.updateBoundingBox();

//    mesh->setVertices(vertexData,numVertices);

	// read index data
	MeshIndexData & id=mesh->openIndexData();
	id.allocate(numIndices);
	uint32_t * indexData = id.data();
	input.read(reinterpret_cast<char *> (indexData), numIndices * sizeof(uint32_t));

	// filter "wrong" vertices
	for(uint32_t i=0;i<numIndices;i+=3){
		uint8_t alpha1 = (vertexData + indexData[i+0] * vd.getVertexSize() + colorAttr.getOffset()) [3];
		uint8_t alpha2 = (vertexData + indexData[i+1] * vd.getVertexSize() + colorAttr.getOffset()) [3];
		uint8_t alpha3 = (vertexData + indexData[i+2] * vd.getVertexSize() + colorAttr.getOffset()) [3];
		if(alpha1 != 255 || alpha2 != 255 || alpha3 != 255  ){
			indexData[i+0]=0;
			indexData[i+1]=0;
			indexData[i+2]=0;
		}

	}
	id.updateIndexRange();

//    mesh->setIndices((uint32_t*)indexData,numIndices);
//	mesh->update();
	return mesh;
}

/**
 * ---|> GenericLoader
 */
Util::GenericAttributeList * StreamerMVBO::loadGeneric(std::istream & input){
	Mesh * m = loadMesh(input);

	auto l = new Util::GenericAttributeList;
	Util::GenericAttributeMap * d = Serialization::createMeshDescription(m);
	l->push_back(d);
	return l;
}

/**
 *  (internal)
 */
uint32_t StreamerMVBO::read(std::istream & in)const{
	uint32_t x;
	in.read(reinterpret_cast<char *> (&x), 4);
	return x;
}

uint8_t StreamerMVBO::queryCapabilities(const std::string & extension) {
	if(extension == fileExtension) {
		return CAP_LOAD_MESH | CAP_LOAD_GENERIC;
	} else {
		return 0;
	}
}

}

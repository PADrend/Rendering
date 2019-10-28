/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "StreamerNGC.h"
#include "Serialization.h"
#include "../Mesh/Mesh.h"
#include "../Mesh/VertexAttributeIds.h"
#include "../Mesh/VertexDescription.h"
#include <Util/GenericAttribute.h>
#include <cstdint>
#include <vector>

/// \todo Show compile error when using a machine without LITTLE-ENDIANness

using namespace Util;
namespace Rendering {
namespace Serialization {

const char * const StreamerNGC::fileExtension = "ngc";


Mesh * StreamerNGC::loadMesh(std::istream & input){
	if(!input.good() || input.eof())
		return nullptr;

	auto mesh = new Mesh;

	uint32_t colorComponentCount;
	uint32_t colorOffset;
	uint32_t colorType;
	uint32_t normalComponentCount;
	uint32_t normalOffset;
	uint32_t normalType;
	uint32_t texCoordComponentCount;
	uint32_t texCoordOffset;
	uint32_t texCoordType;
	uint32_t jumpwidth;
	uint32_t numVertices;
	uint32_t numOfFaces;
	input.read(reinterpret_cast<char*>( & colorComponentCount), sizeof (uint32_t));
	input.read(reinterpret_cast<char*>( & colorOffset), sizeof (uint32_t));
	input.read(reinterpret_cast<char*>( & colorType), sizeof (uint32_t));
	input.read(reinterpret_cast<char*>( & normalComponentCount), sizeof (uint32_t));
	input.read(reinterpret_cast<char*>( & normalOffset), sizeof (uint32_t));
	input.read(reinterpret_cast<char*>( & normalType), sizeof (uint32_t));
	input.read(reinterpret_cast<char*>( & texCoordComponentCount), sizeof (uint32_t));
	input.read(reinterpret_cast<char*>( & texCoordOffset), sizeof (uint32_t));
	input.read(reinterpret_cast<char*>( & texCoordType), sizeof (uint32_t));

	input.read(reinterpret_cast<char*>( & jumpwidth), sizeof (uint32_t));

	input.read(reinterpret_cast<char*>( & numVertices), sizeof (uint32_t));
	input.read(reinterpret_cast<char*>( & numOfFaces), sizeof (uint32_t));
	uint32_t numIndices = numOfFaces*3;

	if(!input.good()){
		std::cout << "\n normalComponentCount:\t"<<normalComponentCount<<"\n";
		std::cout << "colorOffset:\t"<<colorOffset<<"\n";
		std::cout << "colorType:\t"<<colorType<<"\n";

		std::cout << "texCoordComponentCount:\t"<<texCoordComponentCount<<"\n";
		std::cout << "texCoordOffset:\t"<<texCoordOffset<<"\n";
		std::cout << "texCoordType:\t"<<texCoordType<<"\n";

		std::cout << "colorComponentCount:\t"<<colorComponentCount<<"\n";
		std::cout << "colorOffset:\t"<<colorOffset<<"\n";
		std::cout << "colorType:\t"<<colorType<<"\n";
		WARN(" ");
		return nullptr;
	}


	// read vertex data
	std::vector<uint8_t> vBuffer(numVertices*jumpwidth);
	input.read(reinterpret_cast<char*>(vBuffer.data()), vBuffer.size());

	if(!input.good()){
		WARN(" ");
		return nullptr;
	}

	MeshIndexData & id=mesh->openIndexData();
	id.allocate(numIndices);
	input.read(reinterpret_cast<char*>( id.data()), id.dataSize());
	id.updateIndexRange();

	VertexDescription vd;
	const VertexAttribute & posAttr = vd.appendPosition3D();
	const VertexAttribute & normalAttr = vd.appendAttribute(VertexAttributeIds::NORMAL,getAttributeType(normalType),normalComponentCount);
	const VertexAttribute & colorAttr = vd.appendAttribute(VertexAttributeIds::COLOR,getAttributeType(colorType),colorComponentCount);
	const VertexAttribute & tex0Attr = vd.appendAttribute(VertexAttributeIds::TEXCOORD0,getAttributeType(texCoordType),texCoordComponentCount);


	MeshVertexData & vData=mesh->openVertexData();
	vData.allocate(numVertices,vd);
	{
		const uint8_t * source=vBuffer.data()+0;
		uint8_t * target=vData.data()+posAttr.getOffset();
		size_t s=posAttr.getDataSize();
		for(uint32_t i = 0 ; i < numVertices ; ++i){
			std::copy(source,source+s,target);
			source+=jumpwidth;
			target+=vd.getVertexSize();
		}
	}

	// switch position of entries
	if(normalComponentCount>0){
		const uint8_t * source=vBuffer.data()+normalOffset;
		uint8_t * target=vData.data()+normalAttr.getOffset();
		size_t s=normalAttr.getDataSize();
		for(uint32_t i = 0 ; i < numVertices ; ++i){
			std::copy(source,source+s,target);
			source+=jumpwidth;
			target+=vd.getVertexSize();
		}
	}
	if(colorComponentCount>0){
		const uint8_t * source=vBuffer.data()+colorOffset;
		uint8_t * target=vData.data()+colorAttr.getOffset();
		size_t s=colorAttr.getDataSize();
		for(uint32_t i = 0 ; i < numVertices ; ++i){
			std::copy(source,source+s,target);
			source+=jumpwidth;
			target+=vd.getVertexSize();
		}
	}
	if(texCoordComponentCount>0){
		const uint8_t * source=vBuffer.data()+texCoordOffset;
		uint8_t * target=vData.data()+tex0Attr.getOffset();
		size_t s=tex0Attr.getDataSize();
		for(uint32_t i = 0 ; i < numVertices ; ++i){
			std::copy(source,source+s,target);
			source+=jumpwidth;
			target+=vd.getVertexSize();
		}
	}

	vData.updateBoundingBox();

	return mesh;
}

/**
 * ---|> GenericLoader
 */
Util::GenericAttributeList * StreamerNGC::loadGeneric(std::istream & input){
	auto l = new Util::GenericAttributeList;

	while(	Mesh * m = loadMesh(input) ){
		Util::GenericAttributeMap * d = Serialization::createMeshDescription(m);
		l->push_back(d);
		std::cout <<".";
	}
	return l;
}

uint8_t StreamerNGC::queryCapabilities(const std::string & extension) {
	if(extension == fileExtension) {
		return CAP_LOAD_MESH | CAP_LOAD_GENERIC;
	} else {
		return 0;
	}
}

}
}

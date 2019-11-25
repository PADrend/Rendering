/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "StreamerMMF.h"
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

const char * const StreamerMMF::fileExtension = "mmf";

uint32_t StreamerMMF::Reader::read_uint32() {
	uint32_t x;
	in.read(reinterpret_cast<char *> (&x), 4);
	return x;
}

void StreamerMMF::Reader::read(uint8_t * data,size_t count) {
	in.read(reinterpret_cast<char *> (data), count);
}

void StreamerMMF::Reader::skip(uint32_t size) {
	in.seekg(std::ios_base::cur+size);
}

//!	(static)
Mesh * StreamerMMF::loadMesh(std::istream & input) {

//	std::cout << "\nloadMMF...";
	Reader reader(input);

	uint32_t format = reader.read_uint32();
	if(format!=MMF_HEADER)    {
		WARN(std::string("wrong mesh format: ") + Util::StringUtils::toString(format));
		return nullptr;
	}
	uint32_t version = reader.read_uint32();
	if(version>MMF_VERSION)    {
		WARN(std::string("can't read mesh, version to high: ") + Util::StringUtils::toString(version));
		return nullptr;
	}

	auto mesh = new Mesh;
	uint32_t blockType = reader.read_uint32();
	while(blockType != StreamerMMF::MMF_END && input.good()) {
		// blocksize is discarded.
		uint32_t blockSize = reader.read_uint32();
		switch(blockType) {
			case StreamerMMF::MMF_VERTEX_DATA:
				readVertexData(mesh, reader);
				break;
			case StreamerMMF::MMF_INDEX_DATA:
				readIndexData(mesh, reader);
				break;
			default:
				WARN("LoaderMMF::loadMesh: unknown data block found.");
				std::cout << "blockSize:"<<blockSize<<" \n";
				reader.skip(blockSize);
				break;
		}
		blockType = reader.read_uint32();
	}

//	std::cout << "done.\n";
	return mesh;
}

//!	(internal,static)
void StreamerMMF::readVertexData(Mesh * mesh, Reader & in) {
	static const std::string warningPrefix("LoaderMMF::readVertexData: ");

	VertexDescription vd;

	for(uint32_t attrId = in.read_uint32(); attrId != StreamerMMF::MMF_END ; attrId = in.read_uint32()) {
		uint32_t numValues = in.read_uint32();
		uint32_t glType = in.read_uint32();
		uint32_t extLength = in.read_uint32();
		bool normalized = false;
		std::string name;

		switch(attrId) {
			case 0x00: {
					static const std::string s( VertexAttributeIds::POSITION.toString() );
					name = s;
					break;
				}
			case 0x01: {
					static const std::string s( VertexAttributeIds::NORMAL.toString() );
					normalized = true;
					name = s;
					break;
				}
			case 0x02: {
					static const std::string s( VertexAttributeIds::COLOR.toString() );
					normalized = glType == getGLType(Util::TypeConstant::INT8) || glType == getGLType(Util::TypeConstant::UINT8);
					name = s;
					break;
				}
			case 0x06: {
					static const std::string s( VertexAttributeIds::TEXCOORD0.toString() );
					name = s;
					break;
				}
			case 0x07: {
					static const std::string s( VertexAttributeIds::TEXCOORD1.toString() );
					name = s;
					break;
				}
			case MMF_CUSTOM_ATTR_ID: {
					break;
				}
			default: {
					WARN(warningPrefix+"Unknown attribute found.");
					break;
				}
		}


		while(extLength >= 8) {
			uint32_t extBlockType=in.read_uint32();
			uint32_t extBlockSize=in.read_uint32();
			extLength-=8;

			if(extLength>extBlockSize) {
				WARN(warningPrefix+"Error in vertex block");
				FAIL();
			}

			std::vector<uint8_t> data(extBlockSize);
			in.read(data.data(),extBlockSize);
			extLength-=extBlockSize;

			if(extBlockType==MMF_VERTEX_ATTR_EXT_NAME) {
				name.assign(data.begin(), data.end());
				name=name.substr(0,name.find('\0')); // remove additional zeros
			} else {
				WARN(warningPrefix+"Found unsupported ext data, skipping data.");
			}
		}
		if(extLength!=0) {
			WARN(warningPrefix+"Error in vertex block");
			FAIL();
		}
		if(name.empty())
			WARN(warningPrefix+"Found unnamed vertex attribute.");

		vd.appendAttribute(name,getAttributeType(glType),numValues,normalized);
//        vd.setData(index, numValues, glType);

	}
	const uint32_t count = in.read_uint32();
	MeshVertexData & vertices = mesh->openVertexData();
	vertices.allocate(count,vd);

	in.read( vertices.data(), vertices.dataSize());

	vertices.updateBoundingBox();
}

//!	(internal,static)
void StreamerMMF::readIndexData(Mesh * mesh, Reader & in) {
	const uint32_t count = in.read_uint32();
	const uint32_t triangleMode = in.read_uint32();
	mesh->setGLDrawMode(triangleMode);

	// As the use of index data is not stored explicitly in a .mmf-file,
	// if the mesh has no indices, it is assumed that it does not use them. 
	if(count==0){
		mesh->setUseIndexData(false);
	}else{
		mesh->setUseIndexData(true);
		MeshIndexData & indices=mesh->openIndexData();
		indices.allocate(count);

		in.read(reinterpret_cast<uint8_t*>(indices.data()), indices.dataSize());
		indices.updateIndexRange();
	}
}

//! ---|> GenericLoader
Util::GenericAttributeList * StreamerMMF::loadGeneric(std::istream & input) {
	Mesh * m = loadMesh(input);
	if(m == nullptr) {
		return nullptr;
	}
	auto l=new Util::GenericAttributeList;
	Util::GenericAttributeMap * d=Serialization::createMeshDescription(m);
	l->push_back(d);
	return l;
}

//!	(static)
bool StreamerMMF::saveMesh(Mesh * mesh, std::ostream & output) {
	/// Header
	write(output, MMF_HEADER);
	write(output, MMF_VERSION);

	/// VertexData
	MeshVertexData & vertices = mesh->openVertexData();
	const VertexDescription & vd = vertices.getVertexDescription();

	// prepare header
	std::ostringstream headerOut;
	for(const auto & attr : vd.getAttributes()) {
		if(attr.empty())
			continue;
		uint32_t attrId = MMF_CUSTOM_ATTR_ID;
		if(attr.getNameId() == VertexAttributeIds::POSITION){
			attrId=0x00;
		}else if(attr.getNameId() == VertexAttributeIds::NORMAL){
			attrId=0x01;
		}else if(attr.getNameId() == VertexAttributeIds::COLOR){
			attrId=0x02;
		}else if(attr.getNameId() == VertexAttributeIds::TEXCOORD0){
			attrId=0x06;
		}else if(attr.getNameId() == VertexAttributeIds::TEXCOORD1){
			attrId=0x07;
		}
		write(headerOut,attrId);				// attrId
		write(headerOut,attr.getNumValues());	// numValues
		write(headerOut,getGLType(attr.getDataType()));	// dataType
		if(attrId==MMF_CUSTOM_ATTR_ID){
			std::string name(attr.getName());
			while(name.length()%4!=0)// fill name up with \0 until 32bit alignment is reached
				name+='\0';

			write(headerOut,name.length()+8);	// extLength = String + 4 (extType) + 4 (stringLength)
			write(headerOut,MMF_VERTEX_ATTR_EXT_NAME); // extType
			write(headerOut,name.length());		// stringLength
			headerOut.write(name.c_str(),name.length()); // String
		}else{
			write(headerOut,0); 				// extLength = 0
		}
	}
	write(headerOut,MMF_END);
	write(headerOut,vertices.getVertexCount());
	const std::string & header=headerOut.str();

	// write data
	write(output, MMF_VERTEX_DATA);
	write(output,vertices.dataSize()+header.length()); // dataSize
	output.write(header.c_str(),header.length());		// header
	output.write(reinterpret_cast<char *> (vertices.data()), vertices.dataSize()); // data

	/// IndexData Header
	MeshIndexData & indices = mesh->openIndexData();
	write(output, MMF_INDEX_DATA);
	write(output, indices.dataSize()+sizeof(uint32_t)+sizeof(uint32_t)); // indexData length +indexCount +triangleMode
	write(output, indices.getIndexCount());
	write(output, mesh->getGLDrawMode());
	output.write(reinterpret_cast<char *> (indices.data()), indices.dataSize());

	/// final END
	write(output, MMF_END);
	return true;
}



//!	(internal,static)
void StreamerMMF::write(std::ostream & out, uint32_t x) {
	out.write(reinterpret_cast<char *> (&x), 4);
}

uint8_t StreamerMMF::queryCapabilities(const std::string & extension) {
	if(extension == fileExtension) {
		return CAP_LOAD_MESH | CAP_LOAD_GENERIC | CAP_SAVE_MESH;
	} else {
		return 0;
	}
}

}
}

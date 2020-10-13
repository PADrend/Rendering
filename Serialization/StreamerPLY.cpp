/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "StreamerPLY.h"
#include "Serialization.h"
#include "../Mesh/Mesh.h"
#include "../Mesh/VertexAttributeIds.h"
#include "../Mesh/VertexDescription.h"
#include "../Helper.h"
#include <Geometry/Convert.h>
#include <Util/Graphics/Color.h>
#include <Util/GenericAttribute.h>
#include <Util/StringUtils.h>
#include <cstdlib>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>

namespace Rendering {
namespace Serialization {

const char * const StreamerPLY::fileExtension = "ply";

class PLY_Element {
	public:
		static const int16_t MAX_ENTRIES=255;
		static const int16_t MAX_LIST_SIZE=32;

		static const uint8_t TYPE_UNDEFINED=0;
		static const uint8_t TYPE_CHAR=1;
		static const uint8_t TYPE_UCHAR=2;
		static const uint8_t TYPE_SHORT=3;
		static const uint8_t TYPE_USHORT=4;
		static const uint8_t TYPE_INT=5;
		static const uint8_t TYPE_UINT=6;
		static const uint8_t TYPE_FLOAT=7;
		static const uint8_t TYPE_DOUBLE=8;

		enum format_t {
				UNKNOWN,
				ASCII,
				BINARY_BIG_ENDIAN,
				BINARY_LITLLE_ENDIAN
		};

		static uint8_t getTypeId(const std::string & type) {
			if(type=="char" || type=="int8") return TYPE_CHAR;
			else if(type=="uchar" || type=="uint8") return TYPE_UCHAR;
			else if(type=="short" || type=="int16") return TYPE_SHORT;
			else if(type=="ushort" || type=="uint16") return TYPE_USHORT;
			else if(type=="int" || type=="int32") return TYPE_INT;
			else if(type=="uint" || type=="uint32") return TYPE_UINT;
			else if(type=="float" || type=="float32") return TYPE_FLOAT;
			else if(type=="double" || type=="float64") return TYPE_DOUBLE;
			else return TYPE_UNDEFINED;
		};
		static uint8_t getDataSize(uint8_t typeId) {
			static uint8_t sizes[]={0,1,1,2,2,4,4,4,8};
			return sizes[typeId];
//			if(typeId==TYPE_CHAR) return 1;
//			else if(typeId==TYPE_UCHAR) return 1;
//			else if(typeId==TYPE_SHORT) return 2;
//			else if(typeId==TYPE_USHORT) return 2;
//			else if(typeId==TYPE_INT) return 4;
//			else if(typeId==TYPE_UINT) return 4;
//			else if(typeId==TYPE_FLOAT) return 4;
//			else if(typeId==TYPE_DOUBLE) return 8;
//			else return 0;
		};
		static format_t getFormatId(const std::string & format) {
			if(format=="ascii") return ASCII;
			else if(format=="binary_big_endian") return BINARY_BIG_ENDIAN;
			else if(format=="binary_little_endian") return BINARY_LITLLE_ENDIAN;
			else return UNKNOWN;
		};
		// ---
		/***
		 ** PLY_Element::Property
		 **/
		class Property{
			public:
				uint8_t dataType;
				uint8_t countType;
				std::vector<uint8_t> currentData;
				uint8_t currentDataCount;

				Property(uint8_t _dataType, uint8_t _countType = TYPE_UNDEFINED):
						dataType(_dataType), countType(_countType), currentData(), currentDataCount(0) {
				}

				bool isList() const {
					return countType!=TYPE_UNDEFINED;
				}

				void initCurrentData(int16_t numValues) {
					if(numValues != currentDataCount) {
						currentData.resize(numValues * getDataSize(dataType));
						currentDataCount = numValues;
					}
				}

				template<typename t>
				t getCurrentValue(int16_t index=0) const {
					switch (dataType){
						case TYPE_FLOAT:
							return static_cast<t> ((reinterpret_cast<const float *> (currentData.data()))[index]);
						case TYPE_DOUBLE:
							return static_cast<t>((reinterpret_cast<const double *> (currentData.data()))[index]);
						case TYPE_CHAR:
							return static_cast<t>((reinterpret_cast<const int8_t *> (currentData.data()))[index]);
						case TYPE_UCHAR:
							return static_cast<t>((reinterpret_cast<const uint8_t *> (currentData.data()))[index]);
						case TYPE_SHORT:
							return static_cast<t>((reinterpret_cast<const int16_t *> (currentData.data()))[index]);
						case TYPE_USHORT:
							return static_cast<t>((reinterpret_cast<const uint16_t *> (currentData.data()))[index]);
						case TYPE_INT:
							return static_cast<t>((reinterpret_cast<const int32_t *> (currentData.data()))[index]);
						case TYPE_UINT:
							return static_cast<t>((reinterpret_cast<const uint32_t *> (currentData.data()))[index]);
						default:
							return static_cast<t> (0);
					}
				}

				template<typename t>
				void setCurrentValue(int16_t index,t value){
					switch (dataType) {
						case TYPE_FLOAT:
							reinterpret_cast<float *> (currentData.data())[index] = static_cast<float> (value);
							break;
						case TYPE_DOUBLE:
							reinterpret_cast<double *> (currentData.data())[index] = static_cast<double> (value);
							break;
						case TYPE_CHAR:
							reinterpret_cast<int8_t *> (currentData.data())[index] = static_cast<int8_t> (value);
							break;
						case TYPE_UCHAR:
							reinterpret_cast<uint8_t *> (currentData.data())[index] = static_cast<uint8_t> (value);
							break;
						case TYPE_SHORT:
							reinterpret_cast<int16_t *> (currentData.data())[index] = static_cast<int16_t> (value);
							break;
						case TYPE_USHORT:
							reinterpret_cast<uint16_t *> (currentData.data())[index] = static_cast<uint16_t> (value);
							break;
						case TYPE_INT:
							reinterpret_cast<int32_t *> (currentData.data())[index] = static_cast<int32_t> (value);
							break;
						case TYPE_UINT:
							reinterpret_cast<uint32_t *> (currentData.data())[index] = static_cast<uint32_t> (value);
							break;
						default:
							FAIL();
					}
				}

		}; // ------------
		std::string name;
		int count;
	private:
		format_t sourceFormat;

		std::vector<Property> entries;
		std::map<std::string,int16_t> names;
	public:

		/**
		 * [ctor PLY_Element]
		 */
		PLY_Element(std::string _name, format_t _sourceFormat,int _count):
				name(std::move(_name)),count(_count),sourceFormat(_sourceFormat){//,numEntries(0),dataSize(0){
		}

		void addList(const std::string & _countTypeName,const std::string & _typeName,const std::string & _name=""){
			entries.emplace_back(getTypeId(_typeName), getTypeId(_countTypeName));
			if(!_name.empty()) {
				names[_name]=entries.size()-1;
			}
		}

		void addProperty(const std::string & _typeName,const std::string & _name=""){
			addProperty(getTypeId(_typeName),_name);
		}

		void addProperty(uint8_t _typeId,const std::string & _name="") {
			const auto undef = TYPE_UNDEFINED;
			entries.emplace_back(_typeId, undef);
			if(!_name.empty()) {
				names[_name]=entries.size()-1;
			}
		}

		int16_t getPropertyIndex(const std::string & _name) const {
			auto iter = names.find(_name);
			if(iter != names.end())
				return iter->second;
			else
				return -1;
		}

		const Property & getProperty(int16_t index) const {
			return entries[index];
		}
		/**
		 * // dataSize <64 !!!!!!!
		 */
		template<typename t>
		t getBinaryNumber(const uint8_t * data, uint8_t dataSize,
							bool flipBytes) const {
//			std::cout << ":"<<flipBytes<< ((int)data%4)<<(int)dataSize;
			if(flipBytes) {
				uint8_t d[64];
				for(uint_fast8_t i = 0; i < dataSize; ++i) {
					d[i] = data[dataSize - i - 1];
				}
				const t * result = reinterpret_cast<const t *>(d);
				return *result;
			}else{
				uint8_t d[64];
				for(uint_fast8_t i = 0; i < dataSize; ++i)
					d[i] = data[i];
				const t * result = reinterpret_cast<const t *>(d);
				return *result;
			}

		}

		int parseData(const uint8_t * data) {
			int bytesConsumed=0;
			if(sourceFormat == ASCII) {
				const char * start = reinterpret_cast<const char *> (data);
				char * buffer = reinterpret_cast<char *> (const_cast<uint8_t *> (data));
				for(auto & elem : entries) {
					unsigned long num = 1;

					if(elem.isList()) {
						num = strtoul(buffer, &buffer, 10);
					}
					elem.initCurrentData(num);
					for(unsigned long i = 0; i < num; ++i) {
						elem.setCurrentValue<float> (i, strtof(buffer, &buffer));
					}
				}
				bytesConsumed = buffer - start;
			} else {
				bool flipBytes=false; //FORMAT_BINARY_LITLLE_ENDIAN
				if(sourceFormat==BINARY_BIG_ENDIAN)
					flipBytes=true;

				int cursor=0;
				for(auto & e : entries){
					int num=1;

					if(e.isList()){
						int dataSize=getDataSize(e.countType);
						switch (e.countType){
							case TYPE_FLOAT:{
								num=static_cast<int> (getBinaryNumber<float>(data+cursor,dataSize,flipBytes));
								break;
								}
							case TYPE_DOUBLE:{
								num=static_cast<int> (getBinaryNumber<double>(data+cursor,dataSize,flipBytes));
								break;
								}
							case TYPE_CHAR:{
								num=static_cast<int> (getBinaryNumber<int8_t>(data+cursor,dataSize,flipBytes));
								break;
								}
							case TYPE_UCHAR:{
								num=static_cast<int> (getBinaryNumber<uint8_t>(data+cursor,dataSize,flipBytes));
								break;
								}
							case TYPE_SHORT:{
								num=static_cast<int> (getBinaryNumber<int16_t>(data+cursor,dataSize,flipBytes));
								break;
								}
							case TYPE_USHORT:{
								num=static_cast<int> (getBinaryNumber<uint16_t>(data+cursor,dataSize,flipBytes));
								break;
								}
							case TYPE_INT:{
								num=static_cast<int> (getBinaryNumber<int32_t>(data+cursor,dataSize,flipBytes));
								break;
								}
							case TYPE_UINT:{
								num=static_cast<int> (getBinaryNumber<uint32_t>(data+cursor,dataSize,flipBytes));
								break;
								}
							default:
								FAIL();

						}
						cursor+=dataSize;
					}
					e.initCurrentData(num);

					int dataSize=getDataSize(e.dataType);
					switch (e.dataType){
						case TYPE_FLOAT:{
								for(int i=0;i<num;i++){
									e.setCurrentValue<float>(i,getBinaryNumber<float>(data+cursor,dataSize,flipBytes));
									cursor+=dataSize;
								}
								break;
							}
						case TYPE_DOUBLE:{
								for(int i=0;i<num;i++){
									e.setCurrentValue<double>(i,getBinaryNumber<double>(data+cursor,dataSize,flipBytes));
									cursor+=dataSize;
								}
								break;
							}
						case TYPE_CHAR:{
								for(int i=0;i<num;i++){
									e.setCurrentValue<int8_t>(i,getBinaryNumber<int8_t>(data+cursor,dataSize,flipBytes));
									cursor+=dataSize;
								}
								break;
							}
						case TYPE_UCHAR:{
								for(int i=0;i<num;i++){
									e.setCurrentValue<uint8_t>(i,getBinaryNumber<uint8_t>(data+cursor,dataSize,flipBytes));
									cursor+=dataSize;
								}
								break;
							}
						case TYPE_SHORT:{
								for(int i=0;i<num;i++){
									e.setCurrentValue<int16_t>(i,getBinaryNumber<int16_t>(data+cursor,dataSize,flipBytes));
									cursor+=dataSize;
								}
								break;
							}
						case TYPE_USHORT:{
								for(int i=0;i<num;i++){
									e.setCurrentValue<uint16_t>(i,getBinaryNumber<uint16_t>(data+cursor,dataSize,flipBytes));
									cursor+=dataSize;
								}
								break;
							}
						case TYPE_INT:{
								for(int i=0;i<num;i++){
									e.setCurrentValue<int32_t>(i,getBinaryNumber<int32_t>(data+cursor,dataSize,flipBytes));
									cursor+=dataSize;
								}
								break;
							}
						case TYPE_UINT:{
								for(int i=0;i<num;i++){
									e.setCurrentValue<uint32_t>(i,getBinaryNumber<uint32_t>(data+cursor,dataSize,flipBytes));
									cursor+=dataSize;
								}
								break;
							}
						default:
							FAIL();

					}
				}

				bytesConsumed=cursor;


			}
			return bytesConsumed;
		}

};

//-------------------------------------------------------------------------------------------------

Mesh * StreamerPLY::loadMesh(std::istream & input) {
	int cursor=0;

	input.seekg(0, std::ios::end);
	// Use the stream position as size for the new buffer
	std::vector<char> buffer(input.tellg());
	input.seekg(0, std::ios::beg);
	input.read(buffer.data(), buffer.size());

	// ---- read header ---
	if( ! Util::StringUtils::beginsWith(buffer.data(), "ply") ) {
		std::cerr <<" PLYFileLoader Error: Invalid ply header\n";
		return nullptr;
	}
	PLY_Element::format_t format=PLY_Element::ASCII;
	std::string formatVersion="1.0";

	std::vector<PLY_Element> elements;

	while (Util::StringUtils::nextLine(buffer.data(), cursor)) {
		std::string line=Util::StringUtils::getLine(buffer.data() + cursor);
		if(Util::StringUtils::beginsWith(buffer.data() + cursor,"comment"))
			continue;
		else if(Util::StringUtils::beginsWith(buffer.data() + cursor,"element")) {
			std::istringstream s(line,std::istringstream::in);
			std::string dummy;
			std::string elemType;
			uint32_t count=0;
			s>>dummy>>elemType>>count;
			elements.emplace_back(elemType, format, count);

		} else if(Util::StringUtils::beginsWith(buffer.data() + cursor,"property")) {
			std::istringstream s(line,std::istringstream::in);
			std::string dummy;
			std::string dataType;
			s>>dummy>>dataType;
			if(dataType=="list"){
				std::string countType;
				std::string name;
				s>>countType>>dataType>>name;
				if(!elements.empty()) {
					elements.back().addList(countType,dataType,name);
				}
			}else{
				std::string name;
				s>>name;
				if(!elements.empty()) {
					elements.back().addProperty(dataType,name);
				}
			}
		} else if(Util::StringUtils::beginsWith(buffer.data() + cursor,"end_header")) {
			break;
		} else if(Util::StringUtils::beginsWith(buffer.data() + cursor,"format")) {
			std::istringstream is(line,std::istringstream::in);
			std::string dummy;
			std::string sformat;
			is>>dummy>>sformat>>formatVersion;
			format=PLY_Element::getFormatId(sformat);
		} else {
			// ignore unknown Lines
			// std::cout <<"#"<<line<<"\n";
		}
	}
//    std::cout << "\nFormat: "<<format<< " "<<formatVersion<<"\n";

//    std::cout << "\nCursor at "<<(void*)cursor<<(char)buffer[cursor]<<(char)buffer[cursor+1];
	Util::StringUtils::nextLine(buffer.data(), cursor);
//    std::cout << "\nCursor at "<<(void*)cursor<<(char)buffer[cursor]<<(char)buffer[cursor+1];
	if(cursor >= static_cast<int>(buffer.size())) return nullptr;


	// ---- Read Data -----

	auto mesh = new Mesh;

	bool useVertexColor=false;
	bool useVertexNormals=false;
	bool useTex0=false;

	for(auto & e : elements) {
		
		if(e.name=="vertex") {
			uint32_t numVertices=e.count;

			VertexDescription vFormat;
			const VertexAttribute & posAttr = vFormat.appendPosition3D();

			int xIndex=e.getPropertyIndex("x");
			int yIndex=e.getPropertyIndex("y");
			int zIndex=e.getPropertyIndex("z");

			int nxIndex=e.getPropertyIndex("nx");
			int nyIndex=e.getPropertyIndex("ny");
			int nzIndex=e.getPropertyIndex("nz");

			VertexAttribute normalAttr;
			if(nxIndex>=0&&nyIndex>=0&&nzIndex>=0) {
				useVertexNormals=true;
				normalAttr = vFormat.appendNormalByte();
			}
			VertexAttribute tex0Attr;
			int sIndex=e.getPropertyIndex("s");
			int tIndex=e.getPropertyIndex("t");
			if(sIndex>=0&&tIndex>=0) {
				useTex0=true;
				tex0Attr = vFormat.appendTexCoord();
			}
			if(!useTex0) {
				sIndex=e.getPropertyIndex("u");
				tIndex=e.getPropertyIndex("v");
				if(sIndex>=0&&tIndex>=0) {
					useTex0=true;
					tex0Attr = vFormat.appendTexCoord();
				}
			}
			VertexAttribute colorAttr;
			int redIndex=e.getPropertyIndex("red");
			int greenIndex=e.getPropertyIndex("green");
			int blueIndex=e.getPropertyIndex("blue");
			int alphaIndex=e.getPropertyIndex("alpha");
//#ifdef PLY_LOADER_USE_VERTEX_NORMALS
			if(redIndex>=0&&greenIndex>=0&&blueIndex>=0) {
				useVertexColor=true;
				colorAttr = vFormat.appendColorRGBAByte();
			}
//#endif
			int vertexSize=vFormat.getVertexSize();
			const int posOffset=posAttr.getOffset();
			const int normalsOffset=normalAttr.getOffset();
			const int colorOffset=colorAttr.getOffset();
			const int tex0Offset=tex0Attr.getOffset();
			MeshVertexData & vertices=mesh->openVertexData();
			vertices.allocate(numVertices,vFormat);

			uint8_t * vCursor= vertices.data();
			for(uint32_t j=0;j<numVertices;++j) {
//				std::cout << "<"<<j;
				int rowSize=e.parseData(reinterpret_cast<uint8_t *>(buffer.data() + cursor));
				cursor+=rowSize;
//
				*((reinterpret_cast<float *>(vCursor+posOffset))+0)=e.getProperty(xIndex).getCurrentValue<float>();
				*((reinterpret_cast<float *>(vCursor+posOffset))+1)=e.getProperty(yIndex).getCurrentValue<float>();
				*((reinterpret_cast<float *>(vCursor+posOffset))+2)=e.getProperty(zIndex).getCurrentValue<float>();
				if(useVertexNormals) {

					int8_t nx,ny,nz;
					if(e.getProperty(nxIndex).dataType==PLY_Element::TYPE_CHAR){
						nx=e.getProperty(nxIndex).getCurrentValue<int8_t>();
						ny=e.getProperty(nyIndex).getCurrentValue<int8_t>();
						nz=e.getProperty(nzIndex).getCurrentValue<int8_t>();
//                        normal=Geometry::Vec3(values[indices[nxIndex]],values[indices[nyIndex]],values[indices[nzIndex]]);
					}else{
						nx= Geometry::Convert::toSigned<int8_t>(e.getProperty(nxIndex).getCurrentValue<float>());
						ny= Geometry::Convert::toSigned<int8_t>(e.getProperty(nyIndex).getCurrentValue<float>());
						nz= Geometry::Convert::toSigned<int8_t>(e.getProperty(nzIndex).getCurrentValue<float>());
					}
					*((reinterpret_cast<int8_t *>(vCursor+normalsOffset))+0)=nx;
					*((reinterpret_cast<int8_t *>(vCursor+normalsOffset))+1)=ny;
					*((reinterpret_cast<int8_t *>(vCursor+normalsOffset))+2)=nz;
				}
				if(useVertexColor) {
					Util::Color4ub color;
					if(e.getProperty(redIndex).dataType == PLY_Element::TYPE_FLOAT) {
						Util::Color4f floatColor;
						floatColor.setR(e.getProperty(redIndex).getCurrentValue<float>());
						floatColor.setG(e.getProperty(greenIndex).getCurrentValue<float>());
						floatColor.setB(e.getProperty(blueIndex).getCurrentValue<float>());
						if(alphaIndex > 0) {
							floatColor.setA(e.getProperty(alphaIndex).getCurrentValue<float>());
						} else {
							floatColor.setA(1.0f);
						}
						color = Util::Color4ub(floatColor);
					} else { // most likely = TYPE_UCHAR
						color.setR(e.getProperty(redIndex).getCurrentValue<uint8_t>());
						color.setG(e.getProperty(greenIndex).getCurrentValue<uint8_t>());
						color.setB(e.getProperty(blueIndex).getCurrentValue<uint8_t>());
						if(alphaIndex > 0) {
							color.setA(e.getProperty(alphaIndex).getCurrentValue<uint8_t>());
						} else {
							color.setA(255);
						}
					}
					*((reinterpret_cast<uint8_t *> (vCursor + colorOffset)) + 0) = color.getR();
					*((reinterpret_cast<uint8_t *> (vCursor + colorOffset)) + 1) = color.getG();
					*((reinterpret_cast<uint8_t *> (vCursor + colorOffset)) + 2) = color.getB();
					*((reinterpret_cast<uint8_t *> (vCursor + colorOffset)) + 3) = color.getA();
				}
				if(useTex0){
					*((reinterpret_cast<float *>(vCursor+tex0Offset))+0)=e.getProperty(sIndex).getCurrentValue<float>();
					*((reinterpret_cast<float *>(vCursor+tex0Offset))+1)=e.getProperty(tIndex).getCurrentValue<float>();
				}
				if(cursor > static_cast<int>(buffer.size())) {
					std::cerr <<"!!! Buffer overrun!";
					return nullptr;
				}

				vCursor+=vertexSize;
			}
			vertices.updateBoundingBox();
		} else if(e.name=="face") {

			// calculate number of indices
			unsigned int numFaces=e.count;
			int vertex_indicesIndex=e.getPropertyIndex("vertex_indices");

			unsigned int numIndices=0;
#define PLY_LOADER_PROCESS_QUADS 1

#ifdef PLY_LOADER_PROCESS_QUADS
			int cursorBackup=cursor;
			for(unsigned int j=0;j<numFaces;j++) {
				int rowSize = e.parseData(reinterpret_cast<uint8_t *> (buffer.data() + cursor));
				cursor+=rowSize;

				if(cursor > static_cast<int>(buffer.size())) {
					std::cerr <<"!!! Buffer overrun!";
					break;
				}
				int numpoints=e.getProperty(vertex_indicesIndex).currentDataCount;
				if(numpoints==3) numIndices+=3;
				else if(numpoints==4) numIndices+=6;
			}
			cursor=cursorBackup;
#else
			numIndices=numFaces*3;
#endif

			// read indices
			MeshIndexData & indices=mesh->openIndexData();
			indices.allocate(numIndices);

			uint32_t* vIndeces=indices.data();
			uint32_t vertexNr=0;
			for(uint32_t j=0;j<numFaces;++j) {
				int rowSize = e.parseData(reinterpret_cast<uint8_t *> (buffer.data() + cursor));
				cursor+=rowSize;

				if(cursor > static_cast<int>(buffer.size())) {
					std::cerr <<"!!! Buffer overrun!";
					break;
				}
				int numpoints=e.getProperty(vertex_indicesIndex).currentDataCount;

				if(numpoints<3) continue;
				uint32_t p1=e.getProperty(vertex_indicesIndex).getCurrentValue<uint32_t>(0);
				uint32_t p2=e.getProperty(vertex_indicesIndex).getCurrentValue<uint32_t>(1);
				uint32_t p3=e.getProperty(vertex_indicesIndex).getCurrentValue<uint32_t>(2);

				vIndeces[vertexNr++]=p1;
				vIndeces[vertexNr++]=p2;
				vIndeces[vertexNr++]=p3;
////                if(useFaceColor) {
////                    colors[j*3+0]=(float)(values[indices[redIndex]]/256.0);
////                    colors[j*3+1]=(float)(values[indices[greenIndex]]/256.0);
////                    colors[j*3+2]=(float)(values[indices[blueIndex]]/256.0);
////                }
#ifdef PLY_LOADER_PROCESS_QUADS
				if(numpoints==4){
					int p4=e.getProperty(vertex_indicesIndex).getCurrentValue<uint32_t>(3);
					vIndeces[vertexNr++]=p3;
					vIndeces[vertexNr++]=p4;
					vIndeces[vertexNr++]=p1;
				}
#endif
			}
			indices.updateIndexRange();
		} else {
			std::cout <<"\n\n-> "<<e.name<<"\n";
		}
	}

	return mesh;
}

/**
 * ---|> GenericLoader
 */
Util::GenericAttributeList * StreamerPLY::loadGeneric(std::istream & input){
	Mesh * m = loadMesh(input);

	auto l = new Util::GenericAttributeList;
	if( m!=nullptr ){
		Util::GenericAttributeMap * d = Serialization::createMeshDescription(m);
		l->push_back(d);
	}
	return l;
}

bool StreamerPLY::saveMesh(Mesh * mesh, std::ostream & output) {
	VertexDescription vd = mesh->getVertexDescription();

	output << "ply" << std::endl;
	output << "comment minsg 1.0" << std::endl;
	output << "format binary_little_endian 1.0" << std::endl;
	output << "element vertex " << mesh->getVertexCount() << std::endl;

	for(const auto & attr : vd.getAttributes()){
		
		if(attr.empty())
			continue;

		// TODO what is the meaning of this variable? it is never read...
		//bool failure=true;

		std::string prefix=std::string("property ")+getGLTypeString(getGLType(attr.getDataType()))+' ';

		if(attr.getNameId()==VertexAttributeIds::POSITION){
			if(attr.getNumValues()>0)
				output << prefix << "x" << std::endl;
			if(attr.getNumValues()>1)
				output << prefix << "y" << std::endl;
			if(attr.getNumValues()>2)
				output << prefix << "z" << std::endl;
			if(attr.getNumValues()>3)
				output << prefix << "w" << std::endl;
			//if(attr.getNumValues()>4)
				//failure=true;
		}else if(attr.getNameId()==VertexAttributeIds::NORMAL){
			if(attr.getNumValues()>0)
				output << prefix << "nx" << std::endl;
			if(attr.getNumValues()>1)
				output << prefix << "ny" << std::endl;
			if(attr.getNumValues()>2)
				output << prefix << "nz" << std::endl;
			if(attr.getNumValues()>3)
				output << prefix << "nw" << std::endl;
			//if(attr.getNumValues()>4)
				//failure=true;
		}else if(attr.getNameId()==VertexAttributeIds::COLOR){
			if(attr.getNumValues()>0)
				output << prefix << "red" << std::endl;
			if(attr.getNumValues()>1)
				output << prefix << "green" << std::endl;
			if(attr.getNumValues()>2)
				output << prefix << "blue" << std::endl;
			if(attr.getNumValues()>3)
				output << prefix << "alpha" << std::endl;
			//if(attr.getNumValues()>4)
				//failure=true;
		}else if(attr.getNameId()==VertexAttributeIds::TEXCOORD0){
			if(attr.getNumValues()>0)
				output << prefix << "s" << std::endl;
			if(attr.getNumValues()>1)
				output << prefix << "t" << std::endl;
			if(attr.getNumValues()>2)
				output << prefix << "u" << std::endl;
			//if(attr.getNumValues()>4)
				//failure=true;
		}else {
			// export custom attributes
			for(int i=0;i<attr.getNumValues();++i){
				output << prefix << attr.getName() << i << std::endl;
			}
		}

	}
	output << "element face " << mesh->getIndexCount()/3 << std::endl;
	output << "property list uchar int vertex_indices" << std::endl;
	output << "end_header" << std::endl;

	MeshVertexData & vertices = mesh->openVertexData();
	output.write(reinterpret_cast<char *> (vertices.data()), vertices.dataSize());

	char c  = 3;
	char * cp = reinterpret_cast<char *> (mesh->openIndexData().data());

	const size_t entryByteSize=3*sizeof(uint32_t); // =12
	for(unsigned int i=0;i<mesh->getIndexCount()/3;i++) {
		output.write(&c, 1);
		output.write(cp+(i*entryByteSize), entryByteSize);
	}

	return true;
}

uint8_t StreamerPLY::queryCapabilities(const std::string & extension) {
	if(extension == fileExtension) {
		return CAP_LOAD_MESH | CAP_LOAD_GENERIC | CAP_SAVE_MESH;
	} else {
		return 0;
	}
}

}
}

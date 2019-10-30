/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "StreamerMD2.h"
#include "Serialization.h"
#include "../Mesh/VertexAttributeIds.h"
#include "../Mesh/VertexDescription.h"
#include "../MeshUtils/MeshUtils.h"
#include <Geometry/Matrix4x4.h>
#include <Util/GenericAttribute.h>

using namespace Util;
using namespace std;

namespace Rendering {
namespace Serialization {

const char * const StreamerMD2::fileExtension = "md2";

const char * const StreamerMD2::DESCRIPTION_TYPE_KEYFRAME_ANIMATION = "keyFrameAnimation";

const Util::StringIdentifier StreamerMD2::DESCRIPTION_TEXTURE_FILES("textureFiles");
const Util::StringIdentifier StreamerMD2::DESCRIPTION_MESH_INDEX_DATA("meshIndexData");
const Util::StringIdentifier StreamerMD2::DESCRIPTION_KEYFRAMES_DATA("meshFrameData");

const Util::StringIdentifier StreamerMD2::DESCRIPTION_ANIMATIONS("animations");
/*
const char * const StreamerMD2::DESCRIPTION_ANIMATION_NAME = "animationName";
const char * const StreamerMD2::DESCRIPTION_ANIMATION_START_FRAME_INDEX = "animationStart";
const char * const StreamerMD2::DESCRIPTION_ANIMATION_END_FRAME_INDEX = "animationEnd";
const char * const StreamerMD2::DESCRIPTION_ANIMATION_FPS = "animationFps";
*/

// magic number "IDP2" or 844121161
#define MD2IDENT (('2'<<24) + ('P'<<16) + ('D'<<8) + 'I')

// model version
#define MD2VERSION 8

const float StreamerMD2::normals[162][3] =
				{{ -0.525731f,  0.000000f,  0.850651f },
				{ -0.442863f,  0.238856f,  0.864188f },
				{ -0.295242f,  0.000000f,  0.955423f },
				{ -0.309017f,  0.500000f,  0.809017f },
				{ -0.162460f,  0.262866f,  0.951056f },
				{  0.000000f,  0.000000f,  1.000000f },
				{  0.000000f,  0.850651f,  0.525731f },
				{ -0.147621f,  0.716567f,  0.681718f },
				{  0.147621f,  0.716567f,  0.681718f },
				{  0.000000f,  0.525731f,  0.850651f },
				{  0.309017f,  0.500000f,  0.809017f },
				{  0.525731f,  0.000000f,  0.850651f },
				{  0.295242f,  0.000000f,  0.955423f },
				{  0.442863f,  0.238856f,  0.864188f },
				{  0.162460f,  0.262866f,  0.951056f },
				{ -0.681718f,  0.147621f,  0.716567f },
				{ -0.809017f,  0.309017f,  0.500000f },
				{ -0.587785f,  0.425325f,  0.688191f },
				{ -0.850651f,  0.525731f,  0.000000f },
				{ -0.864188f,  0.442863f,  0.238856f },
				{ -0.716567f,  0.681718f,  0.147621f },
				{ -0.688191f,  0.587785f,  0.425325f },
				{ -0.500000f,  0.809017f,  0.309017f },
				{ -0.238856f,  0.864188f,  0.442863f },
				{ -0.425325f,  0.688191f,  0.587785f },
				{ -0.716567f,  0.681718f, -0.147621f },
				{ -0.500000f,  0.809017f, -0.309017f },
				{ -0.525731f,  0.850651f,  0.000000f },
				{  0.000000f,  0.850651f, -0.525731f },
				{ -0.238856f,  0.864188f, -0.442863f },
				{  0.000000f,  0.955423f, -0.295242f },
				{ -0.262866f,  0.951056f, -0.162460f },
				{  0.000000f,  1.000000f,  0.000000f },
				{  0.000000f,  0.955423f,  0.295242f },
				{ -0.262866f,  0.951056f,  0.162460f },
				{  0.238856f,  0.864188f,  0.442863f },
				{  0.262866f,  0.951056f,  0.162460f },
				{  0.500000f,  0.809017f,  0.309017f },
				{  0.238856f,  0.864188f, -0.442863f },
				{  0.262866f,  0.951056f, -0.162460f },
				{  0.500000f,  0.809017f, -0.309017f },
				{  0.850651f,  0.525731f,  0.000000f },
				{  0.716567f,  0.681718f,  0.147621f },
				{  0.716567f,  0.681718f, -0.147621f },
				{  0.525731f,  0.850651f,  0.000000f },
				{  0.425325f,  0.688191f,  0.587785f },
				{  0.864188f,  0.442863f,  0.238856f },
				{  0.688191f,  0.587785f,  0.425325f },
				{  0.809017f,  0.309017f,  0.500000f },
				{  0.681718f,  0.147621f,  0.716567f },
				{  0.587785f,  0.425325f,  0.688191f },
				{  0.955423f,  0.295242f,  0.000000f },
				{  1.000000f,  0.000000f,  0.000000f },
				{  0.951056f,  0.162460f,  0.262866f },
				{  0.850651f, -0.525731f,  0.000000f },
				{  0.955423f, -0.295242f,  0.000000f },
				{  0.864188f, -0.442863f,  0.238856f },
				{  0.951056f, -0.162460f,  0.262866f },
				{  0.809017f, -0.309017f,  0.500000f },
				{  0.681718f, -0.147621f,  0.716567f },
				{  0.850651f,  0.000000f,  0.525731f },
				{  0.864188f,  0.442863f, -0.238856f },
				{  0.809017f,  0.309017f, -0.500000f },
				{  0.951056f,  0.162460f, -0.262866f },
				{  0.525731f,  0.000000f, -0.850651f },
				{  0.681718f,  0.147621f, -0.716567f },
				{  0.681718f, -0.147621f, -0.716567f },
				{  0.850651f,  0.000000f, -0.525731f },
				{  0.809017f, -0.309017f, -0.500000f },
				{  0.864188f, -0.442863f, -0.238856f },
				{  0.951056f, -0.162460f, -0.262866f },
				{  0.147621f,  0.716567f, -0.681718f },
				{  0.309017f,  0.500000f, -0.809017f },
				{  0.425325f,  0.688191f, -0.587785f },
				{  0.442863f,  0.238856f, -0.864188f },
				{  0.587785f,  0.425325f, -0.688191f },
				{  0.688191f,  0.587785f, -0.425325f },
				{ -0.147621f,  0.716567f, -0.681718f },
				{ -0.309017f,  0.500000f, -0.809017f },
				{  0.000000f,  0.525731f, -0.850651f },
				{ -0.525731f,  0.000000f, -0.850651f },
				{ -0.442863f,  0.238856f, -0.864188f },
				{ -0.295242f,  0.000000f, -0.955423f },
				{ -0.162460f,  0.262866f, -0.951056f },
				{  0.000000f,  0.000000f, -1.000000f },
				{  0.295242f,  0.000000f, -0.955423f },
				{  0.162460f,  0.262866f, -0.951056f },
				{ -0.442863f, -0.238856f, -0.864188f },
				{ -0.309017f, -0.500000f, -0.809017f },
				{ -0.162460f, -0.262866f, -0.951056f },
				{  0.000000f, -0.850651f, -0.525731f },
				{ -0.147621f, -0.716567f, -0.681718f },
				{  0.147621f, -0.716567f, -0.681718f },
				{  0.000000f, -0.525731f, -0.850651f },
				{  0.309017f, -0.500000f, -0.809017f },
				{  0.442863f, -0.238856f, -0.864188f },
				{  0.162460f, -0.262866f, -0.951056f },
				{  0.238856f, -0.864188f, -0.442863f },
				{  0.500000f, -0.809017f, -0.309017f },
				{  0.425325f, -0.688191f, -0.587785f },
				{  0.716567f, -0.681718f, -0.147621f },
				{  0.688191f, -0.587785f, -0.425325f },
				{  0.587785f, -0.425325f, -0.688191f },
				{  0.000000f, -0.955423f, -0.295242f },
				{  0.000000f, -1.000000f,  0.000000f },
				{  0.262866f, -0.951056f, -0.162460f },
				{  0.000000f, -0.850651f,  0.525731f },
				{  0.000000f, -0.955423f,  0.295242f },
				{  0.238856f, -0.864188f,  0.442863f },
				{  0.262866f, -0.951056f,  0.162460f },
				{  0.500000f, -0.809017f,  0.309017f },
				{  0.716567f, -0.681718f,  0.147621f },
				{  0.525731f, -0.850651f,  0.000000f },
				{ -0.238856f, -0.864188f, -0.442863f },
				{ -0.500000f, -0.809017f, -0.309017f },
				{ -0.262866f, -0.951056f, -0.162460f },
				{ -0.850651f, -0.525731f,  0.000000f },
				{ -0.716567f, -0.681718f, -0.147621f },
				{ -0.716567f, -0.681718f,  0.147621f },
				{ -0.525731f, -0.850651f,  0.000000f },
				{ -0.500000f, -0.809017f,  0.309017f },
				{ -0.238856f, -0.864188f,  0.442863f },
				{ -0.262866f, -0.951056f,  0.162460f },
				{ -0.864188f, -0.442863f,  0.238856f },
				{ -0.809017f, -0.309017f,  0.500000f },
				{ -0.688191f, -0.587785f,  0.425325f },
				{ -0.681718f, -0.147621f,  0.716567f },
				{ -0.442863f, -0.238856f,  0.864188f },
				{ -0.587785f, -0.425325f,  0.688191f },
				{ -0.309017f, -0.500000f,  0.809017f },
				{ -0.147621f, -0.716567f,  0.681718f },
				{ -0.425325f, -0.688191f,  0.587785f },
				{ -0.162460f, -0.262866f,  0.951056f },
				{  0.442863f, -0.238856f,  0.864188f },
				{  0.162460f, -0.262866f,  0.951056f },
				{  0.309017f, -0.500000f,  0.809017f },
				{  0.147621f, -0.716567f,  0.681718f },
				{  0.000000f, -0.525731f,  0.850651f },
				{  0.425325f, -0.688191f,  0.587785f },
				{  0.587785f, -0.425325f,  0.688191f },
				{  0.688191f, -0.587785f,  0.425325f },
				{ -0.955423f,  0.295242f,  0.000000f },
				{ -0.951056f,  0.162460f,  0.262866f },
				{ -1.000000f,  0.000000f,  0.000000f },
				{ -0.850651f,  0.000000f,  0.525731f },
				{ -0.955423f, -0.295242f,  0.000000f },
				{ -0.951056f, -0.162460f,  0.262866f },
				{ -0.864188f,  0.442863f, -0.238856f },
				{ -0.951056f,  0.162460f, -0.262866f },
				{ -0.809017f,  0.309017f, -0.500000f },
				{ -0.864188f, -0.442863f, -0.238856f },
				{ -0.951056f, -0.162460f, -0.262866f },
				{ -0.809017f, -0.309017f, -0.500000f },
				{ -0.681718f,  0.147621f, -0.716567f },
				{ -0.681718f, -0.147621f, -0.716567f },
				{ -0.850651f,  0.000000f, -0.525731f },
				{ -0.688191f,  0.587785f, -0.425325f },
				{ -0.587785f,  0.425325f, -0.688191f },
				{ -0.425325f,  0.688191f, -0.587785f },
				{ -0.425325f, -0.688191f, -0.587785f },
				{ -0.587785f, -0.425325f, -0.688191f },
				{ -0.688191f, -0.587785f, -0.425325f }
				};


StreamerMD2::StreamerMD2() :
	AbstractRenderingStreamer() {
	//init animation fps data
	standardAnimationFps.insert(make_pair("stand", 9));
	standardAnimationFps.insert(make_pair("run", 10));
	standardAnimationFps.insert(make_pair("attack", 10));
	standardAnimationFps.insert(make_pair("pain", 7));
	standardAnimationFps.insert(make_pair("jump", 7));
	standardAnimationFps.insert(make_pair("flip", 7));
	standardAnimationFps.insert(make_pair("salute", 7));
	standardAnimationFps.insert(make_pair("fallback", 10));
	standardAnimationFps.insert(make_pair("wave", 7));
	standardAnimationFps.insert(make_pair("point", 6));
	standardAnimationFps.insert(make_pair("crstnd", 10));
	standardAnimationFps.insert(make_pair("crstand", 10));
	standardAnimationFps.insert(make_pair("crwalk", 7));
	standardAnimationFps.insert(make_pair("crattak", 10));
	standardAnimationFps.insert(make_pair("crattack", 10));
	standardAnimationFps.insert(make_pair("crpain", 7));
	standardAnimationFps.insert(make_pair("crdeath", 5));
	standardAnimationFps.insert(make_pair("death", 7));
	standardAnimationFps.insert(make_pair("die", 7));
	standardAnimationFps.insert(make_pair("boom", 5));
}

Util::GenericAttributeList * StreamerMD2::loadGeneric(std::istream & input) {

	auto md2Header = new MD2Header;

	input.read(reinterpret_cast<char *>(md2Header), sizeof(MD2Header));

	if( (md2Header->magic != MD2IDENT) && (md2Header->version != MD2VERSION) )
	{
		WARN("Not a valid *.md2 model file!");
		return nullptr;
	}

	auto md2Triangles = new MD2Triangle[md2Header->numTriangles];
	auto md2TexCoords = new MD2TexCoord[md2Header->numTexCoords];
	auto md2Skins = new MD2Skin[md2Header->numSkins];
	auto md2Frames = new MD2Frame[md2Header->numFrames];
	auto md2FrameData = new MD2FrameData[md2Header->numFrames];

	//frame data
	input.seekg(md2Header->offsetFrames);
	for(int i = 0; i < md2Header->numFrames; ++i)
	{
		input.read(reinterpret_cast<char *>(&md2Frames[i]), sizeof(MD2Frame));
		md2FrameData[i].pVertices = new MD2Vertex[md2Header->numVertices];
		input.read(reinterpret_cast<char *>(md2FrameData[i].pVertices), sizeof( MD2Vertex ) * md2Header->numVertices);
	}

	//skins
	input.seekg(md2Header->offsetSkins);
	input.read(reinterpret_cast<char *>(md2Skins), sizeof(MD2Skin) * md2Header->numSkins);

	//tex coords
	input.seekg(md2Header->offsetTexCoords);
	input.read(reinterpret_cast<char *>(md2TexCoords), sizeof(MD2TexCoord) * md2Header->numTexCoords);

	//triangles
	input.seekg(md2Header->offsetTriangles);
	input.read(reinterpret_cast<char *>(md2Triangles), sizeof(MD2Triangle) * md2Header->numTriangles);

	auto description = new Util::GenericAttributeMap;

	description->setString(Serialization::DESCRIPTION_TYPE, StreamerMD2::DESCRIPTION_TYPE_KEYFRAME_ANIMATION);

	//texture files
	std::vector<std::string> textureFiles;
	for (int i = 0; i < md2Header->numSkins; i++) {
		textureFiles.push_back(md2Skins[i].path);
	}
	description->setValue(DESCRIPTION_TEXTURE_FILES, new StreamerMD2::textureFilesWrapper(textureFiles));

	//index data
	MeshIndexData indexData;
	indexData.allocate(md2Header->numTriangles *3);
	{
		uint32_t nVertex=0;
		for(int nTriangle=0; nTriangle < md2Header->numTriangles; nTriangle++) {
			 //reversed order
			indexData[nVertex+0] = nVertex+2;
			indexData[nVertex+1] = nVertex+1;
			indexData[nVertex+2] = nVertex+0;
			nVertex+=3;
		}
	}
	indexData.updateIndexRange();
	description->setValue(DESCRIPTION_MESH_INDEX_DATA, new StreamerMD2::indexDataWrapper(indexData));

	float dSkinResX = static_cast<float>(md2Header->skinWidth);
	float dSkinResY = static_cast<float>(md2Header->skinHeight);

	VertexDescription vertexDescription;
	vertexDescription.appendPosition3D();
	vertexDescription.appendNormalFloat();
	vertexDescription.appendTexCoord();

	//keyframe data
	auto framesData = new StreamerMD2::framesDataWrapper();
	framesData->ref().resize( md2Header->numFrames);

	for(int nFrame=0; nFrame < md2Header->numFrames; nFrame++) {
		MeshVertexData vData;
		vData.allocate(md2Header->numTriangles * 3, vertexDescription);
		float * vertexData = reinterpret_cast<float *> (vData.data());

		for(int nTriangle=0; nTriangle < md2Header->numTriangles; nTriangle++) {
			for(int nVertex=0; nVertex < 3; ++nVertex){
				const short vertexIndex = md2Triangles[nTriangle].vertexIndices[nVertex];

				MD2Vertex curVertex = md2FrameData[nFrame].pVertices[vertexIndex];

				//geom
				*vertexData = curVertex.vertex[0] * md2Frames[nFrame].scale[0] + md2Frames[nFrame].translate[0];
				++vertexData;
				*vertexData = curVertex.vertex[1] * md2Frames[nFrame].scale[1] + md2Frames[nFrame].translate[1];
				++vertexData;
				*vertexData = curVertex.vertex[2] * md2Frames[nFrame].scale[2] + md2Frames[nFrame].translate[2];
				++vertexData;

				//normals
				*vertexData = StreamerMD2::normals[curVertex.lightNormalIndex][0];
				++vertexData;
				*vertexData = StreamerMD2::normals[curVertex.lightNormalIndex][1];
				++vertexData;
				*vertexData = StreamerMD2::normals[curVertex.lightNormalIndex][2];
				++vertexData;

				//tex coords
				const short texCoordIndex = md2Triangles[nTriangle].textureIndices[nVertex];
				const MD2TexCoord & texCoord = md2TexCoords[texCoordIndex];

				*vertexData = static_cast<float>(texCoord.s)/dSkinResX;
				++vertexData;
				*vertexData = 1.0f - static_cast<float>(texCoord.t)/dSkinResY;// Vertical texture coordinate is inverted here, because textures are shown upside-down otherwise.
				++vertexData;
			}
		}

		vData.updateBoundingBox();

		//rotate vertex data
		Geometry::Matrix4x4 transMat;
		transMat.rotate_deg(-90, Geometry::Vec3(1, 0, 0));
		transMat.rotate_deg(90, Geometry::Vec3(0, 0, 1));
		MeshUtils::transform(vData,transMat);

		framesData->ref()[nFrame].swap(vData);
	}
	description->setValue(DESCRIPTION_KEYFRAMES_DATA, framesData);

	//animations
	description->setValue(DESCRIPTION_ANIMATIONS, new StreamerMD2::animationDataWrapper(extractAnimationData(md2Header, md2Frames)));

	auto descriptionList = new Util::GenericAttributeList;
	descriptionList->push_back(description);

	return descriptionList;
}

std::map<std::string, vector<int> > StreamerMD2::extractAnimationData(MD2Header * md2Header, MD2Frame * md2Frames) {

	std::map<std::string, vector<int> > animationsData;

	std::string lastAnimationName;
	int firstFrameIndex = -1;
	for (int i = 0; i < md2Header->numFrames; i++){
		std::string curFrameName(md2Frames[i].name);
		std::string curAnimationName;
		curAnimationName.assign(curFrameName, 0, curFrameName.find_first_of("0123456789"));

		if(curAnimationName != lastAnimationName && !lastAnimationName.empty()){
			std::vector<int> anim;
			anim.push_back(firstFrameIndex);
			anim.push_back(i-1);
			anim.push_back(getFpsByAnimationName(lastAnimationName));
			animationsData.insert(make_pair(lastAnimationName, anim));
			cout << "found animation: " << lastAnimationName << ", start:" << firstFrameIndex << ", end:" << i-1 << ", fps:" << anim[2] << endl;
			firstFrameIndex = i;
		}else if(i == md2Header->numFrames-1){
			if(firstFrameIndex == -1){
				firstFrameIndex = 0;
			}
			std::vector<int> anim;
			anim.push_back(firstFrameIndex);
			anim.push_back(i);
			anim.push_back(getFpsByAnimationName(curAnimationName));
			animationsData.insert(make_pair(curAnimationName, anim));
			cout << "found animation: " << curAnimationName << ", start:" << firstFrameIndex << ", end:" << i << ", fps:" << anim[2] << endl;
		}else if(firstFrameIndex == -1){
			firstFrameIndex = i;
		}

		lastAnimationName = curAnimationName;
	}

	return animationsData;
}

int StreamerMD2::getFpsByAnimationName(const std::string & name) {
	auto it = standardAnimationFps.find(name);
	if (it != standardAnimationFps.end()) {
		return it->second;
	}
	return 10;
}

uint8_t StreamerMD2::queryCapabilities(const std::string & extension) {
	if(extension == fileExtension) {
		return CAP_LOAD_GENERIC;
	} else {
		return 0;
	}
}

}
}

/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef LoaderMD2_H
#define LoaderMD2_H

#include "AbstractRenderingStreamer.h"
#include <Util/StringIdentifier.h>
#include <map>
#include <vector>

namespace Util {
template<typename Type> class WrapperAttribute;
}

namespace Rendering {
class MeshIndexData;
class MeshVertexData;
namespace Serialization {

struct MD2Header
{
   int magic;
   int version;
   int skinWidth;
   int skinHeight;
   int framesize;
   int numSkins;
   int numVertices;
   int numTexCoords;
   int numTriangles;
   int numGlCommands;
   int numFrames;
   int offsetSkins;
   int offsetTexCoords;
   int offsetTriangles;
   int offsetFrames;
   int offsetGlCommands;
   int offsetEnd;
};

struct MD2Vertex
{
	unsigned char vertex[3];
	unsigned char lightNormalIndex;
};

struct MD2Frame
{
   float scale[3];
   float translate[3];
   char name[16];
};

struct MD2FrameData
{
	MD2Vertex* pVertices;
};

struct MD2Triangle
{
   short vertexIndices[3];
   short textureIndices[3];
};

struct MD2Skin
{
   char path[64];
};

struct MD2TexCoord
{
   short s, t;
};

//typedef float Vec3f[3];

class StreamerMD2 : public AbstractRenderingStreamer {
	public:
		StreamerMD2();
		virtual ~StreamerMD2() {
		}

		typedef Util::WrapperAttribute<std::vector<std::string> > textureFilesWrapper;
		typedef Util::WrapperAttribute<MeshIndexData> indexDataWrapper;
		typedef Util::WrapperAttribute<std::vector<MeshVertexData> > framesDataWrapper;
		typedef Util::WrapperAttribute<std::map<std::string, std::vector<int> > > animationDataWrapper;

		//additional descriptions
		RENDERINGAPI static const char * const DESCRIPTION_TYPE_KEYFRAME_ANIMATION;
		RENDERINGAPI static const Util::StringIdentifier DESCRIPTION_TEXTURE_FILES;
		RENDERINGAPI static const Util::StringIdentifier DESCRIPTION_MESH_INDEX_DATA;
		RENDERINGAPI static const Util::StringIdentifier DESCRIPTION_KEYFRAMES_DATA;

		RENDERINGAPI static const Util::StringIdentifier DESCRIPTION_ANIMATIONS;
		/*
		static const char * const DESCRIPTION_ANIMATION_NAME;
		static const char * const DESCRIPTION_ANIMATION_START_FRAME_INDEX;
		static const char * const DESCRIPTION_ANIMATION_END_FRAME_INDEX;
		static const char * const DESCRIPTION_ANIMATION_FPS;
		*/

		RENDERINGAPI static const float normals[162][3];

		//typedef int indexData[];

		/*
		[
			{ // begin first entry
			DESCRIPTION_TYPE 		: "keyframeAnimation",
			DESCRIPTION_FILE 		: "dings.md2",
			DESCRIPTION_TEXTURE_FILES : ["dings.png","dangs.png"]
			DESCRIPTION_MESH_INDEX_DATA : Wrapper f�r IndexData
			DESCRIPTION_KEYFRAMES_DATA :  [ Wrapper f�r VertexData ]
			DESCRIPTION_ANIMATIONS : [ Wrapper f�r AnimationData ]
			}
			// here additional descriptions may follow if more than one object was loaded
		]

		*/

		RENDERINGAPI Util::GenericAttributeList * loadGeneric(std::istream & input) override;

		RENDERINGAPI static uint8_t queryCapabilities(const std::string & extension);
		RENDERINGAPI static const char * const fileExtension;

	protected:
	private:
		RENDERINGAPI std::map<std::string, std::vector<int> > extractAnimationData(MD2Header * md2Header, MD2Frame * md2Frames);
		RENDERINGAPI int getFpsByAnimationName(const std::string & name);
		std::map<std::string, int> standardAnimationFps;


};
}
}

#endif // LoaderMD2_H

/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_STREAMERNGC_H_
#define RENDERING_STREAMERNGC_H_

#include "AbstractRenderingStreamer.h"

namespace Rendering {

/**

	NGC-Format: Tims Containerformat (2010-09-22)
	=============================================

aIFS.read((char*) & aVBOData->colorComponentCount, sizeof (unsigned));
aIFS.read((char*) & aVBOData->colorOffset, sizeof (unsigned));
aIFS.read((char*) & aVBOData->colorType, sizeof (unsigned));
aIFS.read((char*) & aVBOData->normalComponentCount, sizeof (unsigned));
aIFS.read((char*) & aVBOData->normalOffset, sizeof (unsigned));
aIFS.read((char*) & aVBOData->normalType, sizeof (unsigned));
aIFS.read((char*) & aVBOData->texCoordComponentCount, sizeof (unsigned));
aIFS.read((char*) & aVBOData->texCoordOffset, sizeof (unsigned));
aIFS.read((char*) & aVBOData->texCoordType, sizeof (unsigned));

aIFS.read((char*) & aVBOData->jumpwidth, sizeof (unsigned));

aIFS.read((char*) & aVBOData->numOfVerts, sizeof (unsigned));
aIFS.read((char*) & aVBOData->numOfFaces, sizeof (unsigned))

aIFS.read(aVBOData->vertexData, aVBOData->numOfVerts * aVBOData->jumpwidth);
aIFS.read((char*) aVBOData->facesData, aVBOData->numOfFaces * 3 * sizeof(unsigned));


*/
class StreamerNGC : public AbstractRenderingStreamer {
	public:
		StreamerNGC() :
			AbstractRenderingStreamer() {
		}
		virtual ~StreamerNGC() {
		}

		Util::GenericAttributeList * loadGeneric(std::istream & input) override;
		Mesh * loadMesh(std::istream & input) override;

		static uint8_t queryCapabilities(const std::string & extension);
		static const char * const fileExtension;
};

}

#endif /* RENDERING_STREAMERNGC_H_ */

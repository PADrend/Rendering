/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_STREAMERPLY_H_
#define RENDERING_STREAMERPLY_H_

#include "AbstractRenderingStreamer.h"

namespace Rendering {
namespace Serialization {

class StreamerPLY : public AbstractRenderingStreamer {
	public:
		StreamerPLY() :
			AbstractRenderingStreamer() {
		}
		virtual ~StreamerPLY() {
		}

		RENDERINGAPI Util::GenericAttributeList * loadGeneric(std::istream & input) override;
		RENDERINGAPI Mesh * loadMesh(std::istream & input) override;
		RENDERINGAPI bool saveMesh(Mesh * mesh, std::ostream & output) override;

		RENDERINGAPI static uint8_t queryCapabilities(const std::string & extension);
		RENDERINGAPI static const char * const fileExtension;
};

}
}

#endif /* RENDERING_STREAMERPLY_H_ */

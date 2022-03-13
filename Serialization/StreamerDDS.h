/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
  Copyright (C) 2021 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_StreamerDDS_H_
#define RENDERING_StreamerDDS_H_
#ifdef RENDERING_HAVE_LIB_DDS

#include "AbstractRenderingStreamer.h"

namespace Rendering {
namespace Serialization {

/**
 * Loader for compressed DDS/KTX textures.
 */
class StreamerDDS : public AbstractRenderingStreamer {
	public:
		StreamerDDS() :
			AbstractRenderingStreamer() {
		}
		virtual ~StreamerDDS() {
		}

		RENDERINGAPI Util::Reference<Texture> loadTexture(std::istream & input, TextureType, uint32_t numLayers) override;
		
		RENDERINGAPI static uint8_t queryCapabilities(const std::string & extension);
		RENDERINGAPI static const char * const fileExtension;
};

}
}

#endif /* RENDERING_HAVE_LIB_DDS */
#endif /* RENDERING_StreamerDDS_H_ */

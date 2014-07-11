/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_STREAMERPKM_H_
#define RENDERING_STREAMERPKM_H_

#include "AbstractRenderingStreamer.h"

namespace Rendering {

/**
 * Loader for the Ericsson Texture Compression (ETC) format (PKM 10).
 *
 * @see http://devtools.ericsson.com/etc
 * @see http://www.khronos.org/registry/gles/extensions/OES/OES_compressed_ETC1_RGB8_texture.txt
 * @author Benjamin Eikel
 * @date 2011-05-25
 */
class StreamerPKM : public AbstractRenderingStreamer {
	public:
		StreamerPKM() :
			AbstractRenderingStreamer() {
		}
		virtual ~StreamerPKM() {
		}

		Util::Reference<Texture> loadTexture(std::istream & /*input*/, TextureType, uint32_t /*numLayers*/) override;
		
		static uint8_t queryCapabilities(const std::string & extension);
		static const char * const fileExtension;
};

}

#endif /* RENDERING_STREAMERPKM_H_ */

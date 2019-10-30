/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_ABSTRACTRENDERINGSTREAMER_H_
#define RENDERING_ABSTRACTRENDERINGSTREAMER_H_

#include <Util/Serialization/AbstractStreamer.h>
#include <Util/References.h>
#include <Util/Macros.h>
#include <iosfwd>
#include <cstdint>
#include <string>
#include "../Texture/Texture.h"

namespace Rendering {
class Mesh;
namespace Serialization {

/**
 * Interface for classes that are capable of converting between meshes and streams, or textures and streams.
 * Subclasses are allowed to implement only a subset of the functions.
 *
 * @author Benjamin Eikel
 * @date 2011-09-08
 */
class AbstractRenderingStreamer : public Util::Serialization::AbstractStreamer {
	public:
		virtual ~AbstractRenderingStreamer() {
		}

		/**
		 * Load a mesh from the given stream.
		 *
		 * @param input Use the data from the stream beginning at the preset position.
		 * @return Mesh object. The caller is responsible for the memory deallocation.
		 */
		virtual Mesh * loadMesh(std::istream & /*input*/) {
			WARN("Unsupported call for loading a single mesh.");
			return nullptr;
		}

		/**
		 * Save a mesh to the given stream.
		 *
		 * @param mesh Mesh object to save.
		 * @param output Use the stream for writing beginning at the preset position.
		 * @return @c true if successful, @c false otherwise.
		 */
		virtual bool saveMesh(Mesh * /*mesh*/, std::ostream & /*output*/) {
			WARN("Unsupported call for saving a single mesh.");
			return false;
		}

		/**
		 * Load a texture from the given stream.
		 *
		 * @param input Use the data from the stream beginning at the preset position.
		 * @return Texture object. The caller is responsible for the memory deallocation.
		 */
		virtual Util::Reference<Texture> loadTexture(std::istream & /*input*/, TextureType, uint32_t /*numLayers*/) {
			WARN("Unsupported call for loading a single texture.");
			return nullptr;
		}

		/**
		 * Save a texture to the given stream.
		 *
		 * @param texture Texture object to save.
		 * @param output Use the stream for writing beginning at the preset position.
		 * @return @c true if successful, @c false otherwise.
		 */
		virtual bool saveTexture(Texture * /*texture*/, std::ostream & /*output*/) {
			WARN("Unsupported call for saving a single texture.");
			return false;
		}

		static const uint8_t CAP_LOAD_MESH =	1 << 2; //!< Streamer supports the function @a loadMesh
		static const uint8_t CAP_SAVE_MESH =	1 << 3; //!< Streamer supports the function @a saveMesh
		static const uint8_t CAP_LOAD_TEXTURE =	1 << 4; //!< Streamer supports the function @a loadTexture
		static const uint8_t CAP_SAVE_TEXTURE =	1 << 5; //!< Streamer supports the function @a saveTexture

		/**
		 * Check which capabilities are supported for the given file extension.
		 *
		 * @param extension File extension in lower case to check capabilities for.
		 * @return Bitmask consisting of a combination of @a CAP_LOAD_GENERIC and @a CAP_SAVE_GENERIC, @a CAP_LOAD_MESH, @a CAP_SAVE_MESH, @a CAP_LOAD_TEXTURE, @a CAP_SAVE_TEXTURE, or zero.
		 */
		static uint8_t queryCapabilities(const std::string & /*extension*/) {
			return 0;
		}

	protected:
		//! Creation is only possible in subclasses.
		AbstractRenderingStreamer() : Util::Serialization::AbstractStreamer() {
		}
};

}
}

#endif /* RENDERING_ABSTRACTRENDERINGSTREAMER_H_ */

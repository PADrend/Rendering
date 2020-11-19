/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_SERIALIZATION_H
#define RENDERING_SERIALIZATION_H

#include "../Mesh/Mesh.h"
#include "../Texture/TextureType.h"
#include <Util/StringIdentifier.h>
#include <iosfwd>
#include <string>

namespace Util {
class FileName;
class GenericAttributeMap;
class GenericAttributeList;
template<class ObjType> class ReferenceAttribute;
}

namespace Rendering {
class RenderingContext;
class Texture;

/**
 * @brief %Serialization functions for objects (meshes, textures etc.)
 *
 * Conversion between objects and streams.
 * There are static functions for
 * - loading a single mesh from a stream,
 * - saving a single mesh to a stream,
 * - loading a single texture from a stream,
 * - saving a single texture to a stream,
 * - loading a generic description from a stream and
 * - saving a generic description to a stream.
 *
 * @author Benjamin Eikel
 * @date 2011-02-03
 */
namespace Serialization {

typedef Util::ReferenceAttribute<Mesh> MeshWrapper_t;

RENDERINGAPI extern const Util::StringIdentifier DESCRIPTION_TYPE;
RENDERINGAPI extern const char * const DESCRIPTION_TYPE_MESH;
RENDERINGAPI extern const char * const DESCRIPTION_TYPE_MATERIAL;
RENDERINGAPI extern const Util::StringIdentifier DESCRIPTION_FILE;
RENDERINGAPI extern const Util::StringIdentifier DESCRIPTION_DATA;
RENDERINGAPI extern const Util::StringIdentifier DESCRIPTION_TEXTURE_FILE;
RENDERINGAPI extern const Util::StringIdentifier DESCRIPTION_MATERIAL_NAME;
RENDERINGAPI extern const Util::StringIdentifier DESCRIPTION_MATERIAL_AMBIENT;
RENDERINGAPI extern const Util::StringIdentifier DESCRIPTION_MATERIAL_DIFFUSE;
RENDERINGAPI extern const Util::StringIdentifier DESCRIPTION_MATERIAL_SPECULAR;
RENDERINGAPI extern const Util::StringIdentifier DESCRIPTION_MATERIAL_SHININESS;

/**
 * Load a single mesh from the given address.
 * The type of the mesh is determined by the file extension.
 *
 * @param file Address to the file containing the mesh data
 * @return A single mesh
 */
RENDERINGAPI Mesh * loadMesh(const Util::FileName & url);

/**
 * Create a single mesh from the given data.
 * The type of the mesh has to be given as parameter.
 *
 * @param extension File extension specifying the type of the mesh
 * @param data Mesh data
 * @return A single mesh
 */
RENDERINGAPI Mesh * loadMesh(const std::string & extension, const std::string & data);

/**
 * Write a single mesh to the given address.
 * The type of the mesh is determined by the file extension.
 *
 * @param mesh Mesh object to save
 * @param file Address to the file that shall be written
 * @return @c true if successful, @c false otherwise
 */
RENDERINGAPI bool saveMesh(Mesh * mesh, const Util::FileName & url);

/**
 * Write a single mesh to the given stream.
 * The type of the mesh has to be given as parameter.
 *
 * @param mesh Mesh object to save
 * @param extension File extension specifying the type of the mesh
 * @param output Stream to which the data shall be written
 * @return @c true if successful, @c false otherwise
 */
RENDERINGAPI bool saveMesh(Mesh * mesh, const std::string & extension, std::ostream & output);

/**
 * Load a single texture from the given address.
 * The type of the texture is determined by the file extension.
 *
 * @param file Address to the file containing the texture data
 * @return A single texture
 */
RENDERINGAPI Util::Reference<Texture> loadTexture(const Util::FileName & url,  TextureType tType  = TextureType::TEXTURE_2D, uint32_t numLayers=1);

/**
 * Create a single texture from the given data.
 * The type of the texture has to be given as parameter.
 *
 * @param extension File extension specifying the type of the texture
 * @param data Texture data
 * @return A single texture
 */
RENDERINGAPI Util::Reference<Texture> loadTexture(const std::string & extension, const std::string & data, TextureType tType  = TextureType::TEXTURE_2D, uint32_t numLayers=1);

/**
 * Write a single texture to the given address.
 * The type of the texture is determined by the file extension.
 *
 * @param texture Texture object to save
 * @param file Address to the file that shall be written
 * @return @c true if successful, @c false otherwise
 */
RENDERINGAPI bool saveTexture(RenderingContext &context,Texture * texture, const Util::FileName & url);

/**
 * Write a single texture to the given stream.
 * The type of the texture has to be given as parameter.
 *
 * @param texture Texture object to save
 * @param extension File extension specifying the type of the texture
 * @param output Stream to which the data shall be written
 * @return @c true if successful, @c false otherwise
 */
RENDERINGAPI bool saveTexture(RenderingContext &context,Texture * texture, const std::string & extension, std::ostream & output);

/**
 * Load mesh descriptions from the given address.
 * The type of the mesh is determined by the file extension.
 *
 * @param file Address to the file containing the mesh data
 * @return A list of mesh descriptions
 * @note A description list could look like this:
 * @verbatim
 [
  { // begin first entry
   DESCRIPTION_TYPE         : "mesh",          // type of entry (e.g. "mesh", "material", "keyFrameAnimation")
   DESCRIPTION_FILE         : "dings.obj",     // the path to the file the mesh originates from
   DESCRIPTION_DATA         : MeshWrapper_t *, // capsule for the mesh itself
   DESCRIPTION_TEXTURE_FILE : "dings.png"      // (optional) path to a texture file that is used by the mesh
  } // end first entry
  // additional descriptions may follow here if more than one object was loaded
 ]
@endverbatim
 */
RENDERINGAPI Util::GenericAttributeList * loadGeneric(const Util::FileName & url);

/**
 * Create mesh descriptions from the given data.
 * The type of the mesh has to be given as parameter.
 *
 * @param extension File extension specifying the type of the mesh
 * @param data Mesh data
 * @return A list of mesh descriptions
 */
RENDERINGAPI Util::GenericAttributeList * loadGeneric(const std::string & extension, const std::string & data);

/**
 * Helper function which creates a description map for a single mesh.
 *
 * @param mesh The mesh that will be wrapped into the description
 */
RENDERINGAPI Util::GenericAttributeMap * createMeshDescription(Mesh * m);

}

}

#endif // RENDERING_SERIALIZATION_H

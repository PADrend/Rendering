/*
	This file is part of the Rendering library.
	Copyright (C) 2012 Claudius Jähn <claudius@uni-paderborn.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_GENERICATTRIBUTE_STREAMER_H
#define RENDERING_GENERICATTRIBUTE_STREAMER_H

#include <string>

namespace Util {
class GenericAttribute;
class GenericAttributeMap;
template<class ObjType> class ReferenceAttribute;
}
namespace Rendering {
class Mesh;
namespace Serialization {

/*! Adds a handler for Util::_CounterAttribute<Mesh> to Util::GenericAttributeSerialization.
	Should be called at least once before a GenericAttribute is serialized which
	may contain a Mesh.
	\note Texture-Serialization may be added here when needed.
	\note The return value is always true and can be used for static initialization.
*/
RENDERINGAPI bool initGenericAttributeSerialization();

typedef Util::ReferenceAttribute<Mesh> MeshAttribute_t;
RENDERINGAPI const std::string GATypeNameMesh("Mesh");
RENDERINGAPI const std::string embeddedMeshPrefix("$[mmf_b64]");
RENDERINGAPI std::pair<std::string, std::string> serializeGAMesh(const std::pair<const Util::GenericAttribute *,
																	const Util::GenericAttributeMap *> & attributeAndContext);
RENDERINGAPI MeshAttribute_t * unserializeGAMesh(const std::pair<std::string,
													const Util::GenericAttributeMap *> & contentAndContext);

}
}

#endif /* RENDERING_GENERICATTRIBUTE_STREAMER_H */

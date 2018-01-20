/*
	This file is part of the Rendering library.
	Copyright (C) 2012 Claudius Jähn <claudius@uni-paderborn.de>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "GenericAttributeSerialization.h"
#include "Serialization.h"
#include "../Mesh/Mesh.h"
#include <Util/GenericAttribute.h>
#include <Util/GenericAttributeSerialization.h>
#include <Util/Encoding.h>
#include <Util/IO/FileLocator.h>

namespace Rendering {
namespace Serialization {

static const bool renderingAttrStreamerInitialized = initGenericAttributeSerialization();

std::pair<std::string, std::string> serializeGAMesh(const std::pair<const Util::GenericAttribute *, const Util::GenericAttributeMap *> & attributeAndContext) {
	auto meshAttribute = dynamic_cast<const MeshAttribute_t *>(attributeAndContext.first);
	const auto & mesh = meshAttribute->get();
	const auto & filename = mesh->getFileName();
	if(filename.empty()) {
		std::ostringstream meshStream;
		if(saveMesh(mesh, "mmf", meshStream)) {
			const std::string streamString = meshStream.str();
			return std::make_pair(GATypeNameMesh, embeddedMeshPrefix +
									Util::encodeBase64(std::vector<uint8_t>(streamString.begin(), streamString.end())));
		}
		return std::make_pair(GATypeNameMesh, std::string(""));
	} else {
		return std::make_pair(GATypeNameMesh, filename.toString());
	}
}

MeshAttribute_t * unserializeGAMesh(const std::pair<std::string, const Util::GenericAttributeMap *> & contentAndContext) {	
	static const Util::StringIdentifier CONTEXT_FILE_LOCATOR("FileLocator");
	const std::string & s = contentAndContext.first;
	Mesh * mesh = nullptr;
	if(s.compare(0, embeddedMeshPrefix.length(), embeddedMeshPrefix) == 0) {
		const std::vector<uint8_t> meshData = Util::decodeBase64(s.substr(embeddedMeshPrefix.length()));
		mesh = loadMesh("mmf", std::string(meshData.begin(), meshData.end()));
	} else {
		auto filename = Util::FileName(s);
		if(contentAndContext.second->contains(CONTEXT_FILE_LOCATOR)) {
			auto locator = contentAndContext.second->getValue<Util::WrapperAttribute<Util::FileLocator&>>(CONTEXT_FILE_LOCATOR)->get();
			filename = locator.locateFile(filename).second;
		} 
		mesh = loadMesh(filename);
	}
	return mesh == nullptr ? nullptr : new MeshAttribute_t(mesh);
}

bool initGenericAttributeSerialization() {
	static bool serializerRegistered = Util::GenericAttributeSerialization::registerSerializer<MeshAttribute_t>(GATypeNameMesh, serializeGAMesh, unserializeGAMesh);
	return serializerRegistered;
}

}
}

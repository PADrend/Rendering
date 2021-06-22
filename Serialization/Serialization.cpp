/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "Serialization.h"
#include "StreamerMD2.h"
#include "StreamerMMF.h"
#include "StreamerMTL.h"
#include "StreamerMVBO.h"
#include "StreamerNGC.h"
#include "StreamerOBJ.h"
#include "StreamerPKM.h"
#include "StreamerPLY.h"
#include "StreamerXYZ.h"
#include "../Mesh/Mesh.h"
#include "../Texture/Texture.h"
#include "../Texture/TextureUtils.h"
#include <Util/Graphics/Bitmap.h>
#include <Util/Graphics/BitmapUtils.h>
#include <Util/IO/FileName.h>
#include <Util/IO/FileUtils.h>
#include <Util/Serialization/Serialization.h>
#include <Util/GenericAttribute.h>
#include <algorithm>
#include <cctype>
#include <memory>

namespace Rendering {
namespace Serialization {

const Util::StringIdentifier DESCRIPTION_TYPE("type");
const char * const DESCRIPTION_TYPE_MESH("mesh");
const char * const DESCRIPTION_TYPE_MATERIAL("material");
const Util::StringIdentifier DESCRIPTION_FILE("file");
const Util::StringIdentifier DESCRIPTION_DATA("data");
const Util::StringIdentifier DESCRIPTION_TEXTURE_FILE("texture");
const Util::StringIdentifier DESCRIPTION_MATERIAL_NAME("material_name");
const Util::StringIdentifier DESCRIPTION_MATERIAL_AMBIENT("ambient");
const Util::StringIdentifier DESCRIPTION_MATERIAL_DIFFUSE("diffuse");
const Util::StringIdentifier DESCRIPTION_MATERIAL_SPECULAR("specular");
const Util::StringIdentifier DESCRIPTION_MATERIAL_SHININESS("shininess");
/**
 * Return a streamer that supports the requested capability for the given file extension.
 *
 * @param extension File extension (e.g. "ply", "mmf")
 * @param capability Capability that will be requested
 * @return Streamer object that has to be deleted by the caller
 */
static AbstractRenderingStreamer * createStreamer(const std::string & extension, uint8_t capability) {
	std::string lowerExtension(extension);
	std::transform(extension.begin(), extension.end(), lowerExtension.begin(), ::tolower);
	if(StreamerMD2::queryCapabilities(lowerExtension) & capability) {
		return new StreamerMD2;
	} else if(StreamerMMF::queryCapabilities(lowerExtension) & capability) {
		return new StreamerMMF;
	} else if(StreamerMTL::queryCapabilities(lowerExtension) & capability) {
		return new StreamerMTL;
	} else if(StreamerMVBO::queryCapabilities(lowerExtension) & capability) {
		return new StreamerMVBO;
	} else if(StreamerNGC::queryCapabilities(lowerExtension) & capability) {
		return new StreamerNGC;
	} else if(StreamerOBJ::queryCapabilities(lowerExtension) & capability) {
		return new StreamerOBJ;
	} else if(StreamerPKM::queryCapabilities(lowerExtension) & capability) {
		return new StreamerPKM;
	} else if(StreamerPLY::queryCapabilities(lowerExtension) & capability) {
		return new StreamerPLY;
	} else if(StreamerXYZ::queryCapabilities(lowerExtension) & capability) {
		return new StreamerXYZ;
	} else {
		return nullptr;
	}
}

Mesh * loadMesh(const Util::FileName & url) {
	std::unique_ptr<AbstractRenderingStreamer> loader(createStreamer(url.getEnding(), AbstractRenderingStreamer::CAP_LOAD_MESH));
	if(loader.get() == nullptr) {
		WARN("Unsupported file extension \"" + url.getEnding() + "\".");
		return nullptr;
	}
	auto stream = Util::FileUtils::openForReading(url);
	if(!stream) {
		WARN("Error opening stream for reading. Path: " + url.toString());
		return nullptr;
	}
	Util::Reference<Mesh> mesh = loader->loadMesh(*stream);
	if(mesh.isNotNull()) {
		mesh->setFileName(url);
	}
	return mesh.detachAndDecrease();
}

Mesh * loadMesh(const std::string & extension, const std::string & data) {
	std::unique_ptr<AbstractRenderingStreamer> loader(createStreamer(extension, AbstractRenderingStreamer::CAP_LOAD_MESH));
	if(loader.get() == nullptr) {
		WARN("Unsupported file extension \"" + extension + "\".");
		return nullptr;
	}
	std::istringstream stream(data);
	return loader->loadMesh(stream);
}

bool saveMesh(Mesh * mesh, const Util::FileName & url) {
	std::unique_ptr<AbstractRenderingStreamer> saver(createStreamer(url.getEnding(), AbstractRenderingStreamer::CAP_SAVE_MESH));
	if(saver.get() == nullptr) {
		WARN("Unsupported file extension \"" + url.getEnding() + "\".");
		return false;
	}
	auto stream = Util::FileUtils::openForWriting(url);
	if(!stream) {
		WARN("Error opening stream for writing. Path: " + url.toString());
		return false;
	}
	if(!saver->saveMesh(mesh, *stream)) {
		WARN("Saving failed.");
		return false;
	}
	return true;
}

bool saveMesh(Mesh * mesh, const std::string & extension, std::ostream & output) {
	std::unique_ptr<AbstractRenderingStreamer> saver(createStreamer(extension, AbstractRenderingStreamer::CAP_SAVE_MESH));
	if(saver.get() == nullptr) {
		WARN("Unsupported file extension \"" + extension + "\".");
		return false;
	}
	if(!saver->saveMesh(mesh, output)) {
		WARN("Saving failed.");
		return false;
	}
	return true;
}

Util::Reference<Texture> loadTexture(const Util::FileName & url, TextureType tType, uint32_t numLayers,uint32_t desiredChannels) {
	Util::Reference<Texture> texture;
	
	std::unique_ptr<AbstractRenderingStreamer> loader(createStreamer(url.getEnding(), AbstractRenderingStreamer::CAP_LOAD_TEXTURE));
	if( loader.get() ) {	// Rendering streamer found?
		auto stream = Util::FileUtils::openForReading(url);
		if(stream) 
			texture = loader->loadTexture(*stream,tType,numLayers);
		else
			WARN("loadTexture: Error opening stream for reading. Path: " + url.toString());
	} else {	// Try Util::Serialization
		Util::Reference<Util::Bitmap> bitmap = Util::Serialization::loadBitmap(url);
		if(desiredChannels > 0 && bitmap && bitmap->getPixelFormat().getComponentCount() != desiredChannels)
			bitmap = Util::BitmapUtils::expandChannels(*bitmap.get(), desiredChannels);
		if(bitmap)
			texture = TextureUtils::createTextureFromBitmap(*bitmap.get(), tType, numLayers);
	}
	if( texture ) 
		texture->setFileName(url);
	return texture;
}

Util::Reference<Texture> loadTexture(const std::string & extension, const std::string & data, TextureType tType, uint32_t numLayers,uint32_t desiredChannels) {
	std::unique_ptr<AbstractRenderingStreamer> loader(createStreamer(extension, AbstractRenderingStreamer::CAP_LOAD_TEXTURE));
	// Rendering streamer found?
	if(loader.get() != nullptr) {
		std::istringstream stream(data);
		return loader->loadTexture(stream,tType,numLayers);
	} else {
		// Try Util::Serialization.
		Util::Reference<Util::Bitmap> bitmap = Util::Serialization::loadBitmap(extension, data);
		if(desiredChannels > 0 && bitmap && bitmap->getPixelFormat().getComponentCount() != desiredChannels)
			bitmap = Util::BitmapUtils::expandChannels(*bitmap.get(), desiredChannels);
		if(bitmap) 
			return TextureUtils::createTextureFromBitmap(*bitmap.get(), tType, numLayers);
		WARN("Unsupported file extension \"" + extension + "\".");
		return nullptr;
	}
}

bool saveTexture(RenderingContext & context,Texture * texture, const Util::FileName & url) {
	if(!texture){
		WARN("Error saving texture: texture was null");
		return false;
	}
	std::unique_ptr<AbstractRenderingStreamer> saver(createStreamer(url.getEnding(), AbstractRenderingStreamer::CAP_SAVE_TEXTURE));
	if(saver.get() != nullptr){
		auto stream = Util::FileUtils::openForWriting(url);
		if(!stream) {
			WARN("Error opening stream for writing. Path: " + url.toString());
			return false;
		}
		if(!saver->saveTexture(texture, *stream)) {
			WARN("Saving failed.");
			return false;
		}
		return true;
	} else {
		// Try Util::Serialization.
		auto bitmap = TextureUtils::createBitmapFromTexture(context,*texture);
		if(bitmap.isNull()){
			WARN("Error saving texture: internal bitmap was null");
			return false;
		}
		bool success = Util::Serialization::saveBitmap(*bitmap.get(), url);
		if(success) {
			return true;
		}

		WARN("Error saving texture (unsupported file extension \"" + url.getEnding() + "\", or invalid path).");
		return false;
	}

}

bool saveTexture(RenderingContext &context,Texture * texture, const std::string & extension, std::ostream & output) {
	if(!texture){
		WARN("Error saving texture: texture was null");
		return false;
	}
	std::unique_ptr<AbstractRenderingStreamer> saver(createStreamer(extension, AbstractRenderingStreamer::CAP_SAVE_TEXTURE));
	if(saver.get() != nullptr){
		if(!saver->saveTexture(texture, output)) {
			WARN("Saving failed.");
			return false;
		}
		return true;

	} else {
		// Try Util::Serialization.
		auto bitmap = TextureUtils::createBitmapFromTexture(context,*texture);
		if(bitmap.isNull()){
			WARN("Error saving texture: internal bitmap was null");
			return false;
		}
		bool success = Util::Serialization::saveBitmap(*bitmap.get(), extension, output);
		if(success) {
			return true;
		}

		WARN("Unsupported file extension \"" + extension + "\".");
		return false;
	}
}

Util::GenericAttributeList * loadGeneric(const Util::FileName & url) {
	std::unique_ptr<AbstractRenderingStreamer> loader(createStreamer(url.getEnding(), AbstractRenderingStreamer::CAP_LOAD_GENERIC));
	if(loader.get() == nullptr) {
		WARN("Unsupported file extension \"" + url.getEnding() + "\".");
		return nullptr;
	}
	auto stream = Util::FileUtils::openForReading(url);
	if(!stream) {
		WARN("Error opening stream for reading. Path: " + url.toString());
		return nullptr;
	}
	Util::GenericAttributeList * descList = loader->loadGeneric(*stream);
	for (const auto & elem : *descList) {
		Util::GenericAttributeMap * desc = dynamic_cast<Util::GenericAttributeMap *>(elem.get());
		if(desc->getValue(DESCRIPTION_FILE) == nullptr) {
			desc->setString(DESCRIPTION_FILE, url.toString());
		}
	}
	return descList;
}

Util::GenericAttributeList * loadGeneric(const std::string & extension, const std::string & data) {
	std::unique_ptr<AbstractRenderingStreamer> loader(createStreamer(extension, AbstractRenderingStreamer::CAP_LOAD_GENERIC));
	if(loader.get() == nullptr) {
		WARN("Unsupported file extension \"" + extension + "\".");
		return nullptr;
	}
	std::istringstream stream(data);
	Util::GenericAttributeList * descList = loader->loadGeneric(stream);
	return descList;
}

Util::GenericAttributeMap * createMeshDescription(Mesh * m) {
	if(m == nullptr) {
		return nullptr;
	}
	auto d = new Util::GenericAttributeMap;
	d->setString(DESCRIPTION_TYPE, DESCRIPTION_TYPE_MESH);
	d->setValue(DESCRIPTION_DATA, new MeshWrapper_t(m));
	return d;
}

}

}

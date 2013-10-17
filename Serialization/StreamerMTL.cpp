/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "StreamerMTL.h"
#include "Serialization.h"
#include <Util/GenericAttribute.h>
#include <Util/StringUtils.h>

namespace Rendering {

const char * const StreamerMTL::fileExtension = "mtl";

struct State {
		State() :
			offsetX(0.0f), offsetY(0.0f) {
		}
		std::string ambient;
		std::string diffuse;
		std::string specular;
		std::string shininess;

		float offsetX;
		float offsetY;
		std::string texture;
};

//! Add the given state with the given name to the end of the given list.
static void insertState(const std::string & name, const State & state, Util::GenericAttributeList * list) {
	auto desc = new Util::GenericAttributeMap;
	desc->setString(Serialization::DESCRIPTION_TYPE, Serialization::DESCRIPTION_TYPE_MATERIAL);
	desc->setString(Serialization::DESCRIPTION_MATERIAL_NAME, name);
	if (!state.ambient.empty()) {
		desc->setString(Serialization::DESCRIPTION_MATERIAL_AMBIENT, state.ambient);
	}
	if (!state.diffuse.empty()) {
		desc->setString(Serialization::DESCRIPTION_MATERIAL_DIFFUSE, state.diffuse);
	}
	if (!state.specular.empty()) {
		desc->setString(Serialization::DESCRIPTION_MATERIAL_SPECULAR, state.specular);
	}
	if (!state.shininess.empty()) {
		desc->setString(Serialization::DESCRIPTION_MATERIAL_SHININESS, state.shininess);
	}
	static bool first = true;
	if (first && state.texture.find("builtin:unknowntexture.png") != std::string::npos) {
		WARN("Ignoring texture named \"builtin:unknowntexture.png\". This is a workaround for mtl libraries exported by CityEngine.");
		WARN("If you are not such a file, please remove this workaround and inform Ralf Petring.");
		first = false;
	} else if (!state.texture.empty()) {
		desc->setString(Serialization::DESCRIPTION_TEXTURE_FILE, state.texture);
	}
	list->push_back(desc);
}

Util::GenericAttributeList * StreamerMTL::loadGeneric(std::istream & input) {
	auto descriptionList = new Util::GenericAttributeList;

	std::string name;
	State current;
	char chars[256];
	while (input.getline(chars, 256)) {
		char * cursor = chars;
		Util::StringUtils::stepWhitespaces(&cursor);

		if (Util::StringUtils::beginsWith(cursor, "newmtl")) {
			if (!name.empty()) {
				insertState(name, current, descriptionList);
				current = State();
			}
			cursor += 6;
			name = Util::StringUtils::trim(cursor);
		} else if (Util::StringUtils::beginsWith(cursor, "Ka")) {
			cursor += 2;
			current.ambient = Util::StringUtils::trim(cursor);
		} else if (Util::StringUtils::beginsWith(cursor, "Kd")) {
			cursor += 2;
			current.diffuse = Util::StringUtils::trim(cursor);
		} else if (Util::StringUtils::beginsWith(cursor, "Ks")) {
			cursor += 2;
			current.specular = Util::StringUtils::trim(cursor);
		} else if (Util::StringUtils::beginsWith(cursor, "Ns")) {
			cursor += 2;
			current.shininess = Util::StringUtils::trim(cursor);
		} else if (Util::StringUtils::beginsWith(cursor, "map_Kd") || Util::StringUtils::beginsWith(cursor, "map_Ka")) {
			cursor += 6;
			if (Util::StringUtils::beginsWith(cursor, "-o")) {
				cursor += 2;
				current.offsetX = strtof(cursor, &cursor);
				current.offsetY = strtof(cursor, &cursor);
			}
			current.texture = Util::StringUtils::trim(cursor);
		} else if (*cursor != '#' && *cursor != '\0' && *cursor != '\n' && *cursor != '\r') {
			//WARN(std::string("Unknown MTL keyword \"") + *cursor + "\".");
		}
	}

	if (!name.empty()) {
		insertState(name, current, descriptionList);
	}

	return descriptionList;
}

uint8_t StreamerMTL::queryCapabilities(const std::string & extension) {
	if (extension == fileExtension) {
		return CAP_LOAD_GENERIC;
	} else {
		return 0;
	}
}

}

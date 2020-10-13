/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "RenderingState.h"
#include "../Texture/Texture.h"

namespace Rendering {

//----------------

void CameraData::setMatrixCameraToWorld(const Geometry::Matrix4x4f& value) { 
	matrix_cameraToWorld = value;
	matrix_worldToCamera = value.inverse();
	const auto& srt = matrix_cameraToWorld.toSRT();
	position = srt.getTranslation();
	up = srt.getUpVector();
	direction = srt.getDirVector();
}

//----------------

void CameraData::setMatrixCameraToClipping(const Geometry::Matrix4x4f& value) { 
	matrix_cameraToClipping = value;
	matrix_clippingToCamera = value.inverse();
}

//----------------

uint32_t MaterialSet::addMaterial(const MaterialData& material) {
	auto key = std::hash<MaterialData>{}(material);
	const auto& it = materialByHash.find(key);
	if(it != materialByHash.end())
		return it->second;
	uint32_t materialId = static_cast<uint32_t>(materials.size());
	materialByHash[key] = materialId;
	materials.emplace_back(material);
	return materialId;
}

//----------------

bool MaterialSet::hasMaterial(uint32_t materialId) const {
	return materialId < materials.size();
}

//----------------

bool MaterialSet::hasMaterial(const MaterialData& material) const {
	auto key = std::hash<MaterialData>{}(material);
	const auto& it = materialByHash.find(key);
	return it != materialByHash.end();
}

//----------------

const MaterialData& MaterialSet::getMaterial(uint32_t materialId) const {
	return materials[materialId];
}

//----------------

uint32_t LightSet::addLight(const LightData& light) {
	auto key = std::hash<LightData>{}(light);
	const auto& it = lightByHash.find(key);
	if(it != lightByHash.end())
		return it->second;
	uint32_t lightId = static_cast<uint32_t>(lights.size());
	lightByHash[key] = lightId;
	lights.emplace_back(light);
	return lightId;
}

//----------------

bool LightSet::hasLight(uint32_t lightId) const {
	return lightId < lights.size();
}

//----------------

bool LightSet::hasLight(const LightData& light) const {
	auto key = std::hash<LightData>{}(light);
	const auto& it = lightByHash.find(key);
	return it != lightByHash.end();
}

//----------------

const LightData& LightSet::getLight(uint32_t lightId) const {
	return lights[lightId];
}

//----------------

} /* Rendering */
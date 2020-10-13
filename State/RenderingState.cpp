/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "RenderingState.h"
#include "../Texture/Texture.h"
#include "../Shader/Shader.h"
#include "../Shader/Uniform.h"
#include "../Shader/UniformBuffer.h"
#include "../Shader/UniformRegistry.h"
#include <Util/Macros.h>

#include <algorithm>

namespace Rendering {

typedef std::vector<Uniform::UniformName> UniformNameArray_t;
//! (internal)
static UniformNameArray_t createNames(const std::string & prefix, uint8_t number, const std::string & postfix) {
	UniformNameArray_t arr;
	arr.reserve(number);
	for(uint_fast8_t i = 0; i < number; ++i) {
		arr.emplace_back(prefix + static_cast<char>('0' + i) + postfix);
	}
	return arr;
}

static const uint32_t MAX_LIGHTS = 8;

static const Uniform::UniformName UNIFORM_SG_MATRIX_MODEL_TO_CAMERA("sg_matrix_modelToCamera");
static const Uniform::UniformName UNIFORM_SG_MATRIX_CAMERA_TO_CLIPPING("sg_matrix_cameraToClipping");
static const Uniform::UniformName UNIFORM_SG_MATRIX_MODEL_TO_CLIPPING("sg_matrix_modelToClipping");
static const Uniform::UniformName UNIFORM_SG_MATRIX_WORLD_TO_CAMERA("sg_matrix_worldToCamera");
static const Uniform::UniformName UNIFORM_SG_MATRIX_CAMERA_TO_WORLD("sg_matrix_cameraToWorld");
static const Uniform::UniformName UNIFORM_SG_MATRIX_CLIPPING_TO_CAMERA("sg_matrix_clippingToCamera");

static const Uniform::UniformName UNIFORM_SG_LIGHT_COUNT("sg_lightCount");
static const Uniform::UniformName UNIFORM_SG_POINT_SIZE("sg_pointSize");

static const UniformNameArray_t UNIFORM_SG_LIGHT_SOURCES_POSITION(createNames("sg_Light[", MAX_LIGHTS, "].position"));
static const UniformNameArray_t UNIFORM_SG_LIGHT_SOURCES_DIRECTION(createNames("sg_Light[", MAX_LIGHTS, "].direction"));
static const UniformNameArray_t UNIFORM_SG_LIGHT_SOURCES_TYPE(createNames("sg_Light[", MAX_LIGHTS, "].type"));
static const UniformNameArray_t UNIFORM_SG_LIGHT_SOURCES_INTENSITY(createNames("sg_Light[", MAX_LIGHTS, "].intensity"));
static const UniformNameArray_t UNIFORM_SG_LIGHT_SOURCES_RANGE(createNames("sg_Light[", MAX_LIGHTS, "].range"));
static const UniformNameArray_t UNIFORM_SG_LIGHT_SOURCES_COSCONEANGLE(createNames("sg_Light[", MAX_LIGHTS, "].cosConeAngle"));

static const Uniform::UniformName UNIFORM_SG_TEXTURE_ENABLED("sg_textureEnabled");
static const UniformNameArray_t UNIFORM_SG_TEXTURES(createNames("sg_texture", MAX_TEXTURES, ""));
static const Uniform::UniformName UNIFORM_SG_USE_MATERIALS("sg_useMaterials");
static const Uniform::UniformName UNIFORM_SG_MATERIAL_AMBIENT("sg_Material.ambient");
static const Uniform::UniformName UNIFORM_SG_MATERIAL_DIFFUSE("sg_Material.diffuse");
static const Uniform::UniformName UNIFORM_SG_MATERIAL_SPECULAR("sg_Material.specular");
static const Uniform::UniformName UNIFORM_SG_MATERIAL_EMISSION("sg_Material.emission");

//----------------

void CameraData::setMatrixCameraToWorld(const Geometry::Matrix4x4f& value) { 
	matrix_cameraToWorld = value;
	matrix_worldToCamera = value.inverse();
	const auto& srt = matrix_cameraToWorld.toSRT();
	position = srt.getTranslation();
	up = srt.getUpVector();
	direction = srt.getDirVector();
	dirty = true;
}

//----------------

void CameraData::setMatrixCameraToClipping(const Geometry::Matrix4x4f& value) { 
	matrix_cameraToClipping = value;
	matrix_clippingToCamera = value.inverse();
	dirty = true;
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

MaterialData& MaterialSet::getMaterial(uint32_t materialId) {
	return materials[materialId];
}

//----------------

size_t LightSet::addLight(const LightData& light) {
	auto key = Util::hash(light);
	const auto& it = lightByHash.find(key);
	if(it != lightByHash.end())
		return it->second;
	uint32_t lightId = static_cast<uint32_t>(lights.size());
	lightByHash[key] = lightId;
	lights.emplace_back(light);
	dirty = true;
	return key;
}

//----------------

void LightSet::removeLight(size_t lightId) {
	const auto& it = lightByHash.find(lightId);
	if(it == lightByHash.end())
		return;
	if(it->second < lights.size()-1) {
		// swap with last light in lights vector an pop
		auto it2 = lightByHash.find(Util::hash(lights.back()));
		WARN_AND_RETURN_IF(it2 == lightByHash.end(), "Should not happen!",);
		std::iter_swap(lights.begin() + it->second, lights.begin() + it2->second);
		lights.pop_back();
		it2->second = it->second;
	}
	lightByHash.erase(it);
	lights.pop_back();
	dirty = true;
}

//----------------

bool LightSet::hasLight(size_t lightId) const {
	const auto& it = lightByHash.find(lightId);
	return it != lightByHash.end();
}

//----------------

const LightData& LightSet::getLight(size_t lightId) const {
	return lights[lightByHash.at(lightId)];
}

//----------------

void RenderingState::apply(const ShaderRef& shader, bool forced) {
	std::deque<Uniform> uniforms;

	// camera
	bool cc = false;
	if (forced || camera.hasChanged()) {
		cc = true;
		uniforms.emplace_back(UNIFORM_SG_MATRIX_WORLD_TO_CAMERA, camera.getMatrixWorldToCamera());
		uniforms.emplace_back(UNIFORM_SG_MATRIX_CAMERA_TO_WORLD, camera.getMatrixCameraToWorld());
		uniforms.emplace_back(UNIFORM_SG_MATRIX_CAMERA_TO_CLIPPING, camera.getMatrixCameraToClipping());
		uniforms.emplace_back(UNIFORM_SG_MATRIX_CLIPPING_TO_CAMERA, camera.getMatrixClippingToCamera());
		camera.markAsUnchanged();
	}

	// lights
	if (forced || cc || lights.hasChanged()) {

		uniforms.emplace_back(UNIFORM_SG_LIGHT_COUNT, lights.getLightCount());
		for (uint32_t i = 0; i < lights.getLightCount() && i < MAX_LIGHTS; ++i) {
			const auto& light = lights.getLights()[i];

			uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_POSITION[i], camera.getMatrixWorldToCamera().transformPosition(light.getPosition()));
			uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_DIRECTION[i], camera.getMatrixWorldToCamera().transformDirection(light.getDirection()));
			uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_TYPE[i], static_cast<uint32_t>(light.getType()));
			uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_INTENSITY[i], light.getIntensity());
			uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_COSCONEANGLE[i], light.getCosConeAngle());
		}
	}

	// materials
	if (forced || material.hasChanged()) {

		uniforms.emplace_back(UNIFORM_SG_MATERIAL_AMBIENT, material.getAmbient());
		uniforms.emplace_back(UNIFORM_SG_MATERIAL_DIFFUSE, material.getDiffuse());
		uniforms.emplace_back(UNIFORM_SG_MATERIAL_SPECULAR, material.getSpecular());
		uniforms.emplace_back(UNIFORM_SG_MATERIAL_EMISSION, material.getEmission());

		material.markAsUnchanged();
	}

	// modelview

	if (forced || cc || instance.hasChanged()) {
		const auto m = camera.getMatrixCameraToClipping() * instance.getMatrixModelToCamera();
		uniforms.emplace_back(UNIFORM_SG_MATRIX_MODEL_TO_CAMERA, instance.getMatrixModelToCamera());
		uniforms.emplace_back(UNIFORM_SG_MATRIX_MODEL_TO_CLIPPING, m);
		uniforms.emplace_back(UNIFORM_SG_POINT_SIZE, instance.getPointSize());
		instance.markAsUnchanged();
	}

	for(const auto & uniform : uniforms) {
		shader->_getUniformRegistry()->setUniform(uniform, false, forced);
	}
}

//----------------

} /* Rendering */
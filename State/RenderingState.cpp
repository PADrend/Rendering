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

static const UniformNameArray_t UNIFORM_SG_LIGHT_SOURCES_POSITION(createNames("sg_LightSource[", MAX_LIGHTS, "].position"));
static const UniformNameArray_t UNIFORM_SG_LIGHT_SOURCES_DIRECTION(createNames("sg_LightSource[", MAX_LIGHTS, "].direction"));
static const UniformNameArray_t UNIFORM_SG_LIGHT_SOURCES_TYPE(createNames("sg_LightSource[", MAX_LIGHTS, "].type"));
static const UniformNameArray_t UNIFORM_SG_LIGHT_SOURCES_CONSTANT(createNames("sg_LightSource[", MAX_LIGHTS, "].constant"));
static const UniformNameArray_t UNIFORM_SG_LIGHT_SOURCES_LINEAR(createNames("sg_LightSource[", MAX_LIGHTS, "].linear"));
static const UniformNameArray_t UNIFORM_SG_LIGHT_SOURCES_QUADRATIC(createNames("sg_LightSource[", MAX_LIGHTS, "].quadratic"));
static const UniformNameArray_t UNIFORM_SG_LIGHT_SOURCES_AMBIENT(createNames("sg_LightSource[", MAX_LIGHTS, "].ambient"));
static const UniformNameArray_t UNIFORM_SG_LIGHT_SOURCES_DIFFUSE(createNames("sg_LightSource[", MAX_LIGHTS, "].diffuse"));
static const UniformNameArray_t UNIFORM_SG_LIGHT_SOURCES_SPECULAR(createNames("sg_LightSource[", MAX_LIGHTS, "].specular"));
static const UniformNameArray_t UNIFORM_SG_LIGHT_SOURCES_EXPONENT(createNames("sg_LightSource[", MAX_LIGHTS, "].exponent"));
static const UniformNameArray_t UNIFORM_SG_LIGHT_SOURCES_COSCUTOFF(createNames("sg_LightSource[", MAX_LIGHTS, "].cosCutoff"));

static const Uniform::UniformName UNIFORM_SG_TEXTURE_ENABLED("sg_textureEnabled");
static const UniformNameArray_t UNIFORM_SG_TEXTURES(createNames("sg_texture", MAX_TEXTURES, ""));
static const Uniform::UniformName UNIFORM_SG_USE_MATERIALS("sg_useMaterials");
static const Uniform::UniformName UNIFORM_SG_MATERIAL_AMBIENT("sg_Material.ambient");
static const Uniform::UniformName UNIFORM_SG_MATERIAL_DIFFUSE("sg_Material.diffuse");
static const Uniform::UniformName UNIFORM_SG_MATERIAL_SPECULAR("sg_Material.specular");
static const Uniform::UniformName UNIFORM_SG_MATERIAL_EMISSION("sg_Material.emission");
static const Uniform::UniformName UNIFORM_SG_MATERIAL_SHININESS("sg_Material.shininess");

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
	/*if (forced || cc || target.lightsChanged(actual)) {

		target.updateLights(actual);

		uniforms.emplace_back(UNIFORM_SG_LIGHT_COUNT, static_cast<int> (actual.getNumEnabledLights()));

		const uint_fast8_t numEnabledLights = actual.getNumEnabledLights();
		for (uint_fast8_t i = 0; i < numEnabledLights; ++i) {
			const LightParameters & params = actual.getEnabledLight(i);

			target.updateLightParameter(i, params);

			uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_POSITION[i], actual.getMatrix_worldToCamera().transformPosition(params.position) );
			uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_DIRECTION[i], actual.getMatrix_worldToCamera().transformDirection(params.direction) );
			uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_TYPE[i], static_cast<int> (params.type));
			uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_CONSTANT[i], params.constant);
			uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_LINEAR[i], params.linear);
			uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_QUADRATIC[i], params.quadratic);
			uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_AMBIENT[i], params.ambient);
			uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_DIFFUSE[i], params.diffuse);
			uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_SPECULAR[i], params.specular);
			uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_EXPONENT[i], params.exponent);
			uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_COSCUTOFF[i], params.cosCutoff);
		}

		if (forced) { // reset all non-enabled light values
			LightParameters params;
			for (uint_fast8_t i = numEnabledLights; i < MAX_LIGHTS; ++i) {
				target.updateLightParameter(i, params);

				uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_POSITION[i], actual.getMatrix_worldToCamera().transformPosition(params.position) );
				uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_DIRECTION[i], actual.getMatrix_worldToCamera().transformDirection(params.direction) );
				uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_TYPE[i], static_cast<int> (params.type));
				uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_CONSTANT[i], params.constant);
				uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_LINEAR[i], params.linear);
				uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_QUADRATIC[i], params.quadratic);
				uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_AMBIENT[i], params.ambient);
				uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_DIFFUSE[i], params.diffuse);
				uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_SPECULAR[i], params.specular);
				uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_EXPONENT[i], params.exponent);
				uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_COSCUTOFF[i], params.cosCutoff);
			}
		}
	}*/

	// materials
	if (forced || material.hasChanged()) {

		uniforms.emplace_back(UNIFORM_SG_MATERIAL_AMBIENT, material.getAmbient());
		uniforms.emplace_back(UNIFORM_SG_MATERIAL_DIFFUSE, material.getDiffuse());
		uniforms.emplace_back(UNIFORM_SG_MATERIAL_SPECULAR, material.getSpecular());
		uniforms.emplace_back(UNIFORM_SG_MATERIAL_EMISSION, material.getEmission());
		uniforms.emplace_back(UNIFORM_SG_MATERIAL_SHININESS, material.getSpecular().a());

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
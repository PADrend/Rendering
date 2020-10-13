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
static const Uniform::UniformName UNIFORM_SG_VIEWPORT("sg_viewport");

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

// vulkan used right-handed coordinate system with depth-range [-1,1], therefore we need the correction.
static const Geometry::Matrix4x4 ndcCorrection(1,0,0,0,0,-1,0,0,0,0,0.5,0.5,0,0,0,1);

//----------------

CameraData::CameraData(CameraData&& o) {
	matrix_worldToCamera = std::move(o.matrix_worldToCamera);
	matrix_cameraToWorld = std::move(o.matrix_cameraToWorld);
	matrix_cameraToClipping = std::move(o.matrix_cameraToClipping);
	matrix_clippingToCamera = std::move(o.matrix_clippingToCamera);
	position = std::move(o.position);
	direction = std::move(o.direction);
	up = std::move(o.up);
	viewport = std::move(o.viewport);
	dirty = true;
	o.dirty = true;
}

//----------------

CameraData::CameraData(const CameraData& o) {
	matrix_worldToCamera = o.matrix_worldToCamera;
	matrix_cameraToWorld = o.matrix_cameraToWorld;
	matrix_cameraToClipping = o.matrix_cameraToClipping;
	matrix_clippingToCamera = o.matrix_clippingToCamera;
	position = o.position;
	direction = o.direction;
	up = o.up;
	viewport = o.viewport;
	dirty = true;
}

//----------------

CameraData& CameraData::operator=(CameraData&& o) {
	matrix_worldToCamera = std::move(o.matrix_worldToCamera);
	matrix_cameraToWorld = std::move(o.matrix_cameraToWorld);
	matrix_cameraToClipping = std::move(o.matrix_cameraToClipping);
	matrix_clippingToCamera = std::move(o.matrix_clippingToCamera);
	position = std::move(o.position);
	direction = std::move(o.direction);
	up = std::move(o.up);
	viewport = std::move(o.viewport);
	dirty = o.dirty;
	o.dirty = true;
	return *this;
}

//----------------

CameraData& CameraData::operator=(const CameraData& o) {
	dirty |= (*this != o);
	matrix_worldToCamera = o.matrix_worldToCamera;
	matrix_cameraToWorld = o.matrix_cameraToWorld;
	matrix_cameraToClipping = o.matrix_cameraToClipping;
	matrix_clippingToCamera = o.matrix_clippingToCamera;
	position = o.position;
	direction = o.direction;
	up = o.up;
	viewport = o.viewport;
	return *this;
}

//----------------

void CameraData::setMatrixCameraToWorld(const Geometry::Matrix4x4f& value) {
	dirty |= (matrix_cameraToWorld != value);
	matrix_cameraToWorld = value;
	matrix_worldToCamera = value.inverse();
	const auto& srt = matrix_cameraToWorld.toSRT();
	position = srt.getTranslation();
	up = srt.getUpVector();
	direction = srt.getDirVector();
}

//----------------

void CameraData::setMatrixCameraToClipping(const Geometry::Matrix4x4f& value) {
	dirty |= (matrix_cameraToClipping != value);
	matrix_cameraToClipping = value;
	matrix_clippingToCamera = value.inverse();
	dirty = true;
}

//----------------

MaterialData::~MaterialData() = default;

//----------------

MaterialData::MaterialData(MaterialData&& o) {
	model = std::move(o.model);
	ambient = std::move(o.ambient);
	diffuse = std::move(o.diffuse);
	diffuseMap = std::move(o.diffuseMap);
	specular = std::move(o.specular);
	specularMap = std::move(o.specularMap);
	emission = std::move(o.emission);
	emissionMap = std::move(o.emissionMap);
	normalMap = std::move(o.normalMap);
	alphaThreshold = std::move(o.alphaThreshold);
	alphaMask = std::move(o.alphaMask);
	dirty = true;
	o.dirty = true;
}

//----------------

MaterialData::MaterialData(const MaterialData& o) {
	model = o.model;
	ambient = o.ambient;
	diffuse = o.diffuse;
	diffuseMap = o.diffuseMap;
	specular = o.specular;
	specularMap = o.specularMap;
	emission = o.emission;
	emissionMap = o.emissionMap;
	normalMap = o.normalMap;
	alphaThreshold = o.alphaThreshold;
	alphaMask = o.alphaMask;
	dirty = true;
}

//----------------

MaterialData& MaterialData::operator=(MaterialData&& o) {
	model = std::move(o.model);
	ambient = std::move(o.ambient);
	diffuse = std::move(o.diffuse);
	diffuseMap = std::move(o.diffuseMap);
	specular = std::move(o.specular);
	specularMap = std::move(o.specularMap);
	emission = std::move(o.emission);
	emissionMap = std::move(o.emissionMap);
	normalMap = std::move(o.normalMap);
	alphaThreshold = std::move(o.alphaThreshold);
	alphaMask = std::move(o.alphaMask);
	dirty = o.dirty;
	o.dirty = true;
	return *this;
}

//----------------

MaterialData& MaterialData::operator=(const MaterialData& o) {
	dirty |= (*this != o);
	model = o.model;
	ambient = o.ambient;
	diffuse = o.diffuse;
	diffuseMap = o.diffuseMap;
	specular = o.specular;
	specularMap = o.specularMap;
	emission = o.emission;
	emissionMap = o.emissionMap;
	normalMap = o.normalMap;
	alphaThreshold = o.alphaThreshold;
	alphaMask = o.alphaMask;
	return *this;
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

LightData::LightData(LightData&& o) {
	type = std::move(o.type);
	position = std::move(o.position);
	direction = std::move(o.direction);
	intensity = std::move(o.intensity);
	range = std::move(o.range);
	coneAngle = std::move(o.coneAngle);
	cosConeAngle = std::move(o.cosConeAngle);
	dirty = true;
	o.dirty = true;
}

//----------------

LightData::LightData(const LightData& o) {
	type = o.type;
	position = o.position;
	direction = o.direction;
	intensity = o.intensity;
	range = o.range;
	coneAngle = o.coneAngle;
	cosConeAngle = o.cosConeAngle;
	dirty = true;
}

//----------------

LightData& LightData::operator=(LightData&& o) {
	type = std::move(o.type);
	position = std::move(o.position);
	direction = std::move(o.direction);
	intensity = std::move(o.intensity);
	range = std::move(o.range);
	coneAngle = std::move(o.coneAngle);
	cosConeAngle = std::move(o.cosConeAngle);
	dirty = o.dirty;
	o.dirty = true;
	return *this;
}

//----------------

LightData& LightData::operator=(const LightData& o) {
	dirty |= (*this != o);
	type = o.type;
	position = o.position;
	direction = o.direction;
	intensity = o.intensity;
	range = o.range;
	coneAngle = o.coneAngle;
	cosConeAngle = o.cosConeAngle;
	return *this;
}

//----------------

LightSet::LightSet(LightSet&& o) {
	lights = std::move(o.lights);
	lightByHash = std::move(o.lightByHash);
	dirty = true;
	o.dirty = true;
}

//----------------

LightSet::LightSet(const LightSet& o) {
	lights = o.lights;
	lightByHash = o.lightByHash;
	dirty = true;
}

//----------------

LightSet& LightSet::operator=(LightSet&& o) {
	lights = std::move(o.lights);
	lightByHash = std::move(o.lightByHash);
	dirty = o.dirty;
	o.dirty = true;
	return *this;
}

//----------------

LightSet& LightSet::operator=(const LightSet& o) {
	dirty |= (*this != o);
	lights = o.lights;
	lightByHash = o.lightByHash;
	return *this;
}

//----------------

size_t LightSet::addLight(const LightData& light) {
	auto key = Util::hash(light);
	const auto& it = lightByHash.find(key);
	if(it == lightByHash.end()) {
		uint32_t lightId = static_cast<uint32_t>(lights.size());
		lightByHash[key] = lightId;
		lights.emplace_back(light);
		dirty = true;
	}
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

void LightSet::clearDirty() {
	for(auto& light : lights)
		light.clearDirty();
	dirty = false;
}

//----------------

bool LightSet::isDirty() const {
	if(dirty)
		return true;
	for(auto& light : lights)
		if(light.isDirty())
			return true;
	return false;
}

//----------------

InstanceData::InstanceData(InstanceData&& o) {
	matrix_modelToCamera = std::move(o.matrix_modelToCamera);
	materialId = std::move(o.materialId);
	pointSize = std::move(o.pointSize);
	dirty = true;
	o.dirty = true;
}

//----------------

InstanceData::InstanceData(const InstanceData& o) {
	matrix_modelToCamera = o.matrix_modelToCamera;
	materialId = o.materialId;
	pointSize = o.pointSize;
	dirty = true;
}

//----------------

InstanceData& InstanceData::operator=(InstanceData&& o) {
	matrix_modelToCamera = std::move(o.matrix_modelToCamera);
	materialId = std::move(o.materialId);
	pointSize = std::move(o.pointSize);
	dirty = o.dirty;
	o.dirty = true;
	return *this;
}

//----------------

InstanceData& InstanceData::operator=(const InstanceData& o) {
	dirty |= (*this != o);
	matrix_modelToCamera = o.matrix_modelToCamera;
	materialId = o.materialId;
	pointSize = o.pointSize;
	return *this;
}

//----------------

void RenderingState::apply(const ShaderRef& shader, bool forced) {
	std::deque<Uniform> uniforms;

	// camera
	bool cc = false;
	if (forced || camera.isDirty()) {
		cc = true;
		auto corrected = ndcCorrection * camera.getMatrixCameraToClipping();
		auto correctedInv = corrected.inverse();
		uniforms.emplace_back(UNIFORM_SG_MATRIX_WORLD_TO_CAMERA, camera.getMatrixWorldToCamera());
		uniforms.emplace_back(UNIFORM_SG_MATRIX_CAMERA_TO_WORLD, camera.getMatrixCameraToWorld());
		uniforms.emplace_back(UNIFORM_SG_MATRIX_CAMERA_TO_CLIPPING, corrected);
		uniforms.emplace_back(UNIFORM_SG_MATRIX_CLIPPING_TO_CAMERA, correctedInv);
		uniforms.emplace_back(UNIFORM_SG_VIEWPORT, camera.getViewport());
		camera.clearDirty();
	}

	// lights
	if (forced || cc || lights.isDirty()) {

		uniforms.emplace_back(UNIFORM_SG_LIGHT_COUNT, lights.getLightCount());
		for (uint32_t i = 0; i < lights.getLightCount() && i < MAX_LIGHTS; ++i) {
			const auto& light = lights.getLights()[i];

			uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_POSITION[i], camera.getMatrixWorldToCamera().transformPosition(light.getPosition()));
			uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_DIRECTION[i], camera.getMatrixWorldToCamera().transformDirection(light.getDirection()));
			uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_TYPE[i], static_cast<uint32_t>(light.getType()));
			uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_INTENSITY[i], light.getIntensity());
			uniforms.emplace_back(UNIFORM_SG_LIGHT_SOURCES_COSCONEANGLE[i], light.getCosConeAngle());
		}
		lights.clearDirty();
	}

	// materials
	if (forced || material.isDirty()) {

		uniforms.emplace_back(UNIFORM_SG_MATERIAL_AMBIENT, material.getAmbient());
		uniforms.emplace_back(UNIFORM_SG_MATERIAL_DIFFUSE, material.getDiffuse());
		uniforms.emplace_back(UNIFORM_SG_MATERIAL_SPECULAR, material.getSpecular());
		uniforms.emplace_back(UNIFORM_SG_MATERIAL_EMISSION, material.getEmission());

		material.clearDirty();
	}

	// modelview

	if (forced || cc || instance.isDirty()) {
		const auto m = ndcCorrection * camera.getMatrixCameraToClipping() * instance.getMatrixModelToCamera();
		uniforms.emplace_back(UNIFORM_SG_MATRIX_MODEL_TO_CAMERA, instance.getMatrixModelToCamera());
		uniforms.emplace_back(UNIFORM_SG_MATRIX_MODEL_TO_CLIPPING, m);
		uniforms.emplace_back(UNIFORM_SG_POINT_SIZE, instance.getPointSize());
		instance.clearDirty();
	}

	for(const auto & uniform : uniforms) {
		shader->_getUniformRegistry()->setUniform(uniform, false, forced);
	}
}

//----------------

} /* Rendering */
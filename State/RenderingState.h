/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_STATE_RENDERINGSTATE_H_
#define RENDERING_STATE_RENDERINGSTATE_H_

#include <Geometry/Angle.h>
#include <Geometry/Vec3.h>
#include <Geometry/Vec4.h>
#include <Geometry/Matrix4x4.h>

#include <Util/Graphics/Color.h>
#include <Util/References.h>
#include <Util/Utils.h>

#include <vector>
#include <map>

namespace Rendering {
class Texture;
using TextureRef = Util::Reference<Texture>;
class Shader;
using ShaderRef = Util::Reference<Shader>;

template<typename T>
inline size_t calcHash(const T& value) { return std::hash<T>{}(value); }

//==================================================================
// Camera

class CameraData {
public:
	void setMatrixCameraToWorld(const Geometry::Matrix4x4f& value);
	void setMatrixCameraToClipping(const Geometry::Matrix4x4f& value);

	const Geometry::Matrix4x4f& getMatrixWorldToCamera() const { return matrix_worldToCamera; }
	const Geometry::Matrix4x4f& getMatrixCameraToWorld() const { return matrix_cameraToWorld; }
	const Geometry::Matrix4x4f& getMatrixCameraToClipping() const { return matrix_cameraToClipping; }
	const Geometry::Matrix4x4f& getMatrixClippingToCamera() const { return matrix_clippingToCamera; }

	const Geometry::Vec3& getPosition() const { return position; }
	const Geometry::Vec3& getDirection() const { return direction; }
	const Geometry::Vec3& getUp() const { return up; }

	bool operator==(const CameraData& o) const {
		return matrix_cameraToWorld == o.matrix_cameraToWorld && matrix_cameraToClipping == o.matrix_cameraToClipping;
	}
	bool operator!=(const CameraData& o) const { return !(*this == o); }

	void markAsUnchanged() { hash = calcHash(*this); dirty = false; }
	bool hasChanged() const { return dirty ? hash != calcHash(*this) : false; }
private:
	Geometry::Matrix4x4f matrix_worldToCamera;
	Geometry::Matrix4x4f matrix_cameraToWorld;
	Geometry::Matrix4x4f matrix_cameraToClipping;
	Geometry::Matrix4x4f matrix_clippingToCamera;

	Geometry::Vec3 position;
	Geometry::Vec3 direction;
	Geometry::Vec3 up;

	bool dirty = true;
	size_t hash = 0;
};

//==================================================================
// Material

enum class ShadingModel {
	Shadeless = 0,
	Phong,
	MetalRoughness,
	SpecularGlossiness,
};

//-------------

class MaterialData {
public:
	void setAmbient(const Util::Color4f& color) { ambient = color; dirty = true; }
	void setDiffuse(const Util::Color4f& color) { diffuse = color; diffuseMap = nullptr; dirty = true; }
	void setDiffuse(const TextureRef& texture) { diffuseMap = texture; dirty = true; }
	void setSpecular(const Util::Color4f& color) { specular = color; specularMap = nullptr; dirty = true; }
	void setSpecular(const TextureRef& texture) { specularMap = texture; dirty = true; }
	void setEmission(const Util::Color4f& color) { emission = color; emissionMap = nullptr; dirty = true; }
	void setEmission(const TextureRef& texture) { emissionMap = texture; dirty = true; }
	void setEmissionIntensity(float value) { emission.a(value); dirty = true; }
	void setNormal(const TextureRef& texture) { normalMap = texture; dirty = true; }
	void setAlphaThreshold(float value) { alphaThreshold = value; dirty = true; }
	void setAlphaMaskEnabled(bool value) { alphaMask = value; dirty = true; }
	void setShadingModel(ShadingModel value) { model = value; dirty = true; }

	const Util::Color4f& getAmbient() const { return ambient; }
	const Util::Color4f& getDiffuse() const { return diffuse; }
	const TextureRef& getDiffuseMap() const { return diffuseMap; }
	const Util::Color4f& getSpecular() const { return specular; }
	const TextureRef& getSpecularMap() const { return specularMap; }
	const Util::Color4f& getEmission() const { return emission; }
	const TextureRef& getEmissionMap() const { return emissionMap; }
	const TextureRef& getNormalMap() const { return normalMap; }
	float getAlphaThreshold() const { return alphaThreshold; }
	bool isAlphaMaskEnabled() const { return alphaMask; }
	ShadingModel getShadingModel() const { return model; }

	bool operator==(const MaterialData& o) const {
		return model == o.model &&
			ambient == o.ambient &&
			diffuse == o.diffuse &&
			diffuseMap == o.diffuseMap &&
			specular == o.specular &&
			specularMap == o.specularMap &&
			emission == o.emission &&
			emissionMap == o.emissionMap &&
			normalMap == o.normalMap &&
			alphaThreshold == o.alphaThreshold &&
			alphaMask == o.alphaMask;
	}
	bool operator!=(const MaterialData& o) const { return !(*this == o); }

	void markAsUnchanged() { hash = calcHash(*this); dirty = false; }
	bool hasChanged() const { return dirty ? hash != calcHash(*this) : false; }
private:
	ShadingModel model = ShadingModel::Phong;
	Util::Color4f ambient = {0,0,0,0};
	Util::Color4f diffuse = {1,1,1,1};
	TextureRef diffuseMap;
	Util::Color4f specular = {0,0,0,0};
	TextureRef specularMap;
	Util::Color4f emission = {0,0,0,1};
	TextureRef emissionMap;
	TextureRef normalMap;
	float alphaThreshold = 0.5;
	bool alphaMask = false;

	bool dirty = true;
	size_t hash = 0;
};

//-------------

class MaterialSet {
public:
	uint32_t addMaterial(const MaterialData& material);
	bool hasMaterial(uint32_t materialId) const;
	bool hasMaterial(const MaterialData& material) const;
	MaterialData& getMaterial(uint32_t materialId);
	void clear() { materials.clear(); materialByHash.clear(); }
	const std::vector<MaterialData>& getMaterials() const { return materials; }

	bool operator==(const MaterialSet& o) const { return materials == o.materials; }
	bool operator!=(const MaterialSet& o) const { return materials != o.materials; }
private:
	std::vector<MaterialData> materials;
	std::map<size_t,uint32_t> materialByHash;
};

//==================================================================
// Light

enum class LightType {
	Directional = 1,
	Point,
	Spot,
};

//-------------

class LightData {
public:
	void setType(LightType value) { type = value; dirty = true; }
	void setPosition(const Geometry::Vec3& value) { position = value; dirty = true; }
	void setDirection(const Geometry::Vec3& value) { direction = value; dirty = true; }
	void setIntensity(const Util::Color4f& value) { intensity = value; dirty = true; }
	void setConeAngle(const Geometry::Angle& value) { coneAngle = value; cosConeAngle = std::cos(coneAngle.rad()); dirty = true; }
	void setRange(float value) { range = value; dirty = true; }

	LightType getType() const { return type; }
	const Geometry::Vec3& getPosition() const { return position; }
	const Geometry::Vec3& getDirection() const { return direction; }
	const Util::Color4f& getIntensity() const { return intensity; }
	float getRange() const { return range; }
	const Geometry::Angle& getConeAngle() const { return coneAngle; }
	float getCosConeAngle() const { return cosConeAngle; }

	bool operator==(const LightData& o) const {
		return type == o.type &&
			position == o.position &&
			direction == o.direction &&
			intensity == o.intensity &&
			range == o.range &&
			coneAngle == o.coneAngle;
	}
	bool operator!=(const LightData& o) const { return !(*this == o); }

	void markAsUnchanged() { hash = calcHash(*this); dirty = false; }
	bool hasChanged() const { return dirty ? hash != calcHash(*this) : false; }
private:
	LightType type;
	Geometry::Vec3 position = {0,0,0};
	Geometry::Vec3 direction = {0,-1,0};
	Util::Color4f intensity = {1,1,1,1};
	float range = -1;
	Geometry::Angle coneAngle = Geometry::Angle::deg(20.0);
	float cosConeAngle = std::cos(Geometry::Angle::deg(20.0).rad());

	bool dirty = true;
	size_t hash = 0;
};

//-------------

class LightSet {
public:
	uint32_t addLight(const LightData& light);
	bool hasLight(uint32_t lightId) const;
	bool hasLight(const LightData& light) const;
	const LightData& getLight(uint32_t lightId) const;
	void clear() { lights.clear(); lightByHash.clear(); }
	const std::vector<LightData>& getLights() const { return lights; }

	bool operator==(const LightSet& o) const { return lights == o.lights; }
	bool operator!=(const LightSet& o) const { return lights != o.lights; }
private:
	std::vector<LightData> lights;
	std::map<size_t,uint32_t> lightByHash;
};

//==================================================================
// Instance

class InstanceData {
public:
	void setMatrixModelToCamera(const Geometry::Matrix4x4f& value) { matrix_modelToCamera = value; dirty = true; }
	void multMatrixModelToCamera(const Geometry::Matrix4x4f& value) { matrix_modelToCamera *= value; dirty = true; }
	void setMaterialId(uint32_t value) { materialId = value; dirty = true; }
	void setPointSize(float value) { pointSize = value; dirty = true; }

	const Geometry::Matrix4x4f& getMatrixModelToCamera() const { return matrix_modelToCamera; }
	uint32_t getMaterialId() const { return materialId; }
	float getPointSize() const { return pointSize; }

	bool operator==(const InstanceData& o) const {
		return matrix_modelToCamera == o.matrix_modelToCamera &&
			materialId == o.materialId &&
			pointSize == o.pointSize;
	}
	bool operator!=(const InstanceData& o) const { return !(*this == o); }

	void markAsUnchanged() { hash = calcHash(*this); dirty = false; }
	bool hasChanged() const { return dirty ? hash != calcHash(*this) : false; }
private:
	Geometry::Matrix4x4f matrix_modelToCamera;
	uint32_t materialId = 0;
	float pointSize = 1.0;

	bool dirty = true;
	size_t hash = 0;
};


//==================================================================
// RenderingState

class RenderingState {
public:
	CameraData& getCamera() { return camera; }
	//MaterialSet& getMaterials() { return materials; }
	MaterialData& getMaterial() { return material; }
	LightSet& getLights() { return lights; }
	InstanceData& getInstance() { return instance; }

	void setMaterial(const MaterialData& mat) { material = mat; }

	void apply(const ShaderRef& shader, bool forced=false);

	bool operator==(const RenderingState& o) const {
		return camera == o.camera &&
			material == o.material &&
			lights == o.lights;
	}
	bool operator!=(const RenderingState& o) const { return !(*this == o); }
private:
	CameraData camera;
	//MaterialSet materials;
	MaterialData material;
	LightSet lights;
	InstanceData instance; // For now, only one instance is supported
};

//-------------

} /* Rendering */

//==================================================================
// Hashing


namespace std {

//-------------

template <> struct hash<Geometry::Vec3> {
	std::size_t operator()(const Geometry::Vec3& value) const {
		std::size_t result = 0;
		Util::hash_combine(result, value.getX());
		Util::hash_combine(result, value.getY());
		Util::hash_combine(result, value.getZ());
		return result;
	}
};

//-------------

template <> struct hash<Geometry::Matrix4x4> {
	std::size_t operator()(const Geometry::Matrix4x4& value) const {
		std::size_t result = 0;
		for(uint_fast8_t i=0; i<16; ++i)
			Util::hash_combine(result, value.at(i));
		return result;
	}
};

//-------------

template <> struct hash<Rendering::CameraData> {
	std::size_t operator()(const Rendering::CameraData& data) const {
		std::size_t result = 0;
		Util::hash_combine(result, data.getMatrixWorldToCamera());
		Util::hash_combine(result, data.getMatrixCameraToClipping());
		return result;
	}
};

//-------------

template <> struct hash<Rendering::MaterialData> {
	std::size_t operator()(const Rendering::MaterialData& data) const {
		std::size_t result = 0;
		Util::hash_combine(result, data.getAmbient());

		if(data.getDiffuseMap())
			Util::hash_combine(result, data.getDiffuseMap().get());
		else
			Util::hash_combine(result, data.getDiffuse());

		if(data.getSpecularMap())
			Util::hash_combine(result, data.getSpecularMap().get());
		else
			Util::hash_combine(result, data.getSpecular());

		if(data.getEmissionMap()) {
			Util::hash_combine(result, data.getEmission().a());
			Util::hash_combine(result, data.getEmissionMap().get());	
		} else {
			Util::hash_combine(result, data.getEmission());
		}
		Util::hash_combine(result, data.getNormalMap().get());
		Util::hash_combine(result, data.isAlphaMaskEnabled());
		Util::hash_combine(result, data.isAlphaMaskEnabled() ? data.getAlphaThreshold() : 0.0f);
		Util::hash_combine(result, data.getShadingModel());
		return result;
	}
};

//-------------

template <> struct hash<Rendering::LightData> {
	std::size_t operator()(const Rendering::LightData& data) const {
		std::size_t result = 0;
		Util::hash_combine(result, data.getType());
		Util::hash_combine(result, data.getPosition());
		Util::hash_combine(result, data.getIntensity());
		Util::hash_combine(result, data.getRange());
		if(data.getType() != Rendering::LightType::Point)
			Util::hash_combine(result, data.getDirection());			
		if(data.getType() == Rendering::LightType::Spot)
			Util::hash_combine(result, data.getConeAngle().deg());
		return result;
	}
};

//-------------

template <> struct hash<Rendering::InstanceData> {
	std::size_t operator()(const Rendering::InstanceData& data) const {
		std::size_t result = 0;
		Util::hash_combine(result, data.getMatrixModelToCamera());
		Util::hash_combine(result, data.getMaterialId());
		Util::hash_combine(result, data.getPointSize());
		return result;
	}
};

} /* std */
#endif /* end of include guard: RENDERING_STATE_RENDERINGSTATE_H_ */
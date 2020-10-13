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
private:
	Geometry::Matrix4x4f matrix_worldToCamera;
	Geometry::Matrix4x4f matrix_cameraToWorld;
	Geometry::Matrix4x4f matrix_cameraToClipping;
	Geometry::Matrix4x4f matrix_clippingToCamera;

	Geometry::Vec3 position;
	Geometry::Vec3 direction;
	Geometry::Vec3 up;
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
	void setAmbient(const Util::Color4f& color) { ambient = color; }
	void setDiffuse(const Util::Color4f& color) { diffuse = color; diffuseMap = nullptr; }
	void setDiffuse(const TextureRef& texture) { diffuseMap = texture; }
	void setSpecular(const Util::Color4f& color) { specular = color; specularMap = nullptr; }
	void setSpecular(const TextureRef& texture) { specularMap = texture; }
	void setEmission(const Util::Color4f& color) { emission = color; emissionMap = nullptr; }
	void setEmission(const TextureRef& texture) { emissionMap = texture; }
	void setEmissionIntensity(float value) { emission.a(value); }
	void setNormal(const TextureRef& texture) { normalMap = texture; }
	void setAlphaThreshold(float value) { alphaThreshold = value; }
	void setAlphaMaskEnabled(bool value) { alphaMask = value; }
	void setShadingModel(ShadingModel value) { model = value; }

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
private:
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
	ShadingModel model = ShadingModel::Phong;
};

//-------------

class MaterialSet {
public:
	uint32_t addMaterial(const MaterialData& material);
	bool hasMaterial(uint32_t materialId) const;
	bool hasMaterial(const MaterialData& material) const;
	const MaterialData& getMaterial(uint32_t materialId) const;
	void clear() { materials.clear(); materialByHash.clear(); }
	const std::vector<MaterialData>& getMaterials() const { return materials; }
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
	void setType(LightType value) { type = value; }
	void setPosition(const Geometry::Vec3& value) { position = value; }
	void setDirection(const Geometry::Vec3& value) { direction = value; }
	void setIntensity(const Util::Color4f& value) { intensity = value; }
	void setConeAngle(const Geometry::Angle& value) { coneAngle = value; cosConeAngle = std::cos(coneAngle.rad()); }
	void setRange(float value) { range = value; }

	LightType getType() const { return type; }
	const Geometry::Vec3& getPosition() const { return position; }
	const Geometry::Vec3& getDirection() const { return direction; }
	const Util::Color4f& getIntensity() const { return intensity; }
	float getRange() const { return range; }
	const Geometry::Angle& getConeAngle() const { return coneAngle; }
	float getCosConeAngle() const { return cosConeAngle; }
private:
	LightType type;
	Geometry::Vec3 position = {0,0,0};
	Geometry::Vec3 direction = {0,-1,0};
	Util::Color4f intensity = {1,1,1,1};
	float range = -1;
	Geometry::Angle coneAngle = Geometry::Angle::deg(20.0);
	float cosConeAngle = std::cos(Geometry::Angle::deg(20.0).rad());
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
private:
	std::vector<LightData> lights;
	std::map<size_t,uint32_t> lightByHash;
};

//==================================================================
// Instance

class InstanceData {
public:
	void setMatrixModelToCamera(const Geometry::Matrix4x4f& value) { matrix_modelToCamera = value; }
	void multMatrixModelToCamera(const Geometry::Matrix4x4f& value) { matrix_modelToCamera *= value; }
	void setMaterialId(uint32_t value) { materialId = value; }
	void setPointSize(float value) { pointSize = value; }

	const Geometry::Matrix4x4f& getMatrixModelToCamera() const { return matrix_modelToCamera; }
	uint32_t getMaterialId() const { return materialId; }
	float getPointSize() const { return pointSize; }
private:
	Geometry::Matrix4x4f matrix_modelToCamera;
	uint32_t materialId = 0;
	float pointSize = 1.0;
};


//==================================================================
// RenderingState

class RenderingState {
public:
	CameraData& getCamera() { return camera; }
	MaterialSet& getMaterials() { return materials; }
	LightSet& getLights() { return lights; }
	InstanceData& getInstance() { return instance; }
private:
	CameraData camera;
	MaterialSet materials;
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
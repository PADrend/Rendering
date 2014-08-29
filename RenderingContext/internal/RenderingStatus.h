/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2013 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_DATA_H_
#define RENDERING_DATA_H_

#include "../RenderingParameters.h"
#include "../../Texture/TextureType.h"
#include <Geometry/Matrix4x4.h>
#include <bitset>
#include <cassert>
#include <deque>
#include <vector>
#include <cstdint>

namespace Rendering {
class Shader;

//! (internal) Used by shaders and the renderingContext to track the state of shader (and openGL) dependent properties.
class RenderingStatus {

	//!	@name General
	//	@{
	private:
		Shader * shader;
		bool initialized;

	public:
		explicit RenderingStatus(Shader * _shader = nullptr) : 
			shader(_shader), 
			initialized(false),
			checkNumber_matrixCameraWorld(0),
			matrix_worldToCamera(),
			matrix_cameraToWorld(),
			lightsCheckNumber(0),
			lights(),
			lightsEnabled(0),
			materialCheckNumber(0),
			materialEnabled(false),
			material(),
			matrix_modelToCameraCheckNumber(0),
			matrix_modelToCamera(),
			pointParameters(),
			matrix_cameraToClipCheckNumber(0),
			matrix_cameraToClipping(),
			textureUnitUsagesCheckNumber(0),
			textureUnitParams(MAX_TEXTURES, std::make_pair(TexUnitUsageParameter::DISABLED,TextureType::TEXTURE_2D)) {
		}
		Shader * getShader() 						{	return shader;	}
		bool isInitialized()const					{	return initialized;	}
		void markInitialized()						{	initialized=true;	}
	//	@}

	// -------------------------------

	//!	@name Camera Matrix
	//	@{
	private:
		uint32_t checkNumber_matrixCameraWorld;
		Geometry::Matrix4x4f matrix_worldToCamera;
		Geometry::Matrix4x4f matrix_cameraToWorld;
	public:
		bool matrixCameraToWorldChanged(const RenderingStatus & actual) const {
			return (checkNumber_matrixCameraWorld == actual.checkNumber_matrixCameraWorld) ? false :
					matrix_cameraToWorld != actual.matrix_cameraToWorld;
		}
		const Geometry::Matrix4x4f & getMatrix_cameraToWorld() const 	{	return matrix_cameraToWorld;	}
		const Geometry::Matrix4x4f & getMatrix_worldToCamera() const	{	return matrix_worldToCamera;	}
		void setMatrix_cameraToWorld(const Geometry::Matrix4x4f & eyeToWorld) {
			matrix_cameraToWorld = eyeToWorld;
			matrix_worldToCamera = eyeToWorld.inverse();
			++checkNumber_matrixCameraWorld;
		}
		void updateMatrix_cameraToWorld(const RenderingStatus & actual) {
			matrix_cameraToWorld = actual.matrix_cameraToWorld;
			matrix_worldToCamera = actual.matrix_worldToCamera;
			checkNumber_matrixCameraWorld = actual.checkNumber_matrixCameraWorld;
		}
	//	@}

	// ------

	//!	@name Lights
	//	@{
	public:
		static const uint8_t MAX_LIGHTS = 8;
	private:
		uint32_t lightsCheckNumber;
		//! Storage of light parameters.
		LightParameters lights[MAX_LIGHTS];

		//! Status of the lights (1 = enabled, 0 = disabled).
		std::bitset<MAX_LIGHTS> lightsEnabled;
	public:
		//! Return the number of lights that are currently enabled.
		uint8_t getNumEnabledLights() const {
			return lightsEnabled.count();
		}
		//! Of the lights that are enabled, return the one with the given index.
		const LightParameters & getEnabledLight(uint8_t index) const {
			assert(index < getNumEnabledLights());
			uint_fast8_t pos = 0;
			// Find first enabled light.
			while(!lightsEnabled[pos]) {
				++pos;
			}
			// Find next enabled light, until the index'th enabled light is found.
			while(index > 0) {
				while(!lightsEnabled[pos]) {
					++pos;
				}
				--index;
				++pos;
			}
			return lights[pos];
		}
		//! Enable the light given by its parameters. Return the number that can be used to disable it.
		uint8_t enableLight(const LightParameters & light) {
			assert(getNumEnabledLights() < MAX_LIGHTS);
			uint_fast8_t pos = 0;
			while(lightsEnabled[pos]) {
				++pos;
			}
			++lightsCheckNumber;
			lights[pos] = light;
			lightsEnabled[pos] = true;
			return pos;
		}
		//! Disable the light with the given number.
		void disableLight(uint8_t lightNumber) {
			assert(lightsEnabled[lightNumber]);
			++lightsCheckNumber;
			lightsEnabled[lightNumber] = false;
		}
		//! Return @c true, if the light with the given light number is enabled.
		bool isLightEnabled(uint8_t lightNumber) const {
			return lightsEnabled[lightNumber];
		}
		bool lightsChanged(const RenderingStatus & actual) const {
			if (lightsCheckNumber == actual.lightsCheckNumber)
				return false;
			if (lightsEnabled != actual.lightsEnabled)
				return true;
			for (uint_fast8_t i = 0; i < getNumEnabledLights(); ++i) {
				if (getEnabledLight(i) != actual.getEnabledLight(i)) {
					return true;
				}
			}
			return false;
		}
		void updateLights(const RenderingStatus & actual) {
			lightsEnabled = actual.lightsEnabled;
			lightsCheckNumber = actual.lightsCheckNumber;
		}
		void updateLightParameter(uint8_t lightNumber, const LightParameters & light) {
			assert(lightNumber < MAX_LIGHTS);
			lights[lightNumber] = light;
		}
	//	@}

	// ------

	//!	@name Materials
	//	@{
	private:
		uint32_t materialCheckNumber;
		bool materialEnabled;
		MaterialParameters material;

	public:
		bool isMaterialEnabled()const								{	return materialEnabled;	}
		const MaterialParameters & getMaterialParameters()const	{	return material;	}
		bool materialChanged(const RenderingStatus & actual) const {
			return (materialCheckNumber == actual.materialCheckNumber) ? false :
						(materialEnabled != actual.materialEnabled || material != actual.material);
		}
		void setMaterial(const MaterialParameters & mat) {
			material = mat;
			materialEnabled = true;
			++materialCheckNumber;
		}
		void updateMaterial(const RenderingStatus & actual) {
			materialEnabled=actual.materialEnabled;
			material=actual.material;
			materialCheckNumber=actual.materialCheckNumber;
		}
		void disableMaterial() {
			materialEnabled = false;
			++materialCheckNumber;
		}
	//	@}

	// ------

	//!	@name Modelview Matrix
	//	@{
	private:
		uint32_t matrix_modelToCameraCheckNumber;
		Geometry::Matrix4x4f matrix_modelToCamera;

	public:
		const Geometry::Matrix4x4f & getMatrix_modelToCamera() const 				{	return matrix_modelToCamera;	}
		void setMatrix_modelToCamera(const Geometry::Matrix4x4f & matrix) {
			matrix_modelToCamera = matrix;
			++matrix_modelToCameraCheckNumber;
		}
		bool matrix_modelToCameraChanged(const RenderingStatus & actual) const {
			return (matrix_modelToCameraCheckNumber == actual.matrix_modelToCameraCheckNumber) ? false :
					matrix_modelToCamera != actual.matrix_modelToCamera;
		}
		void multModelViewMatrix(const Geometry::Matrix4x4f & matrix) {
			matrix_modelToCamera *= matrix;
			++matrix_modelToCameraCheckNumber;
		}
		void updateModelViewMatrix(const RenderingStatus & actual) {
			matrix_modelToCamera = actual.matrix_modelToCamera;
			matrix_modelToCameraCheckNumber = actual.matrix_modelToCameraCheckNumber;
		}
	//	@}

	// ------

	//!	@name Point
	//	@{
	private:
		PointParameters pointParameters;
	public:
		bool pointParametersChanged(const RenderingStatus & actual) const {
			return pointParameters != actual.pointParameters;
		}
		const PointParameters & getPointParameters() const {
			return pointParameters;
		}
		void setPointParameters(const PointParameters & p) {
			pointParameters = p;
		}
	//	@}

	// ------

	//!	@name Projection Matrix
	//	@{
	private:
		uint32_t matrix_cameraToClipCheckNumber;
		Geometry::Matrix4x4f matrix_cameraToClip;

	public:
		void setMatrix_cameraToClipping(const Geometry::Matrix4x4f & matrix) {
			matrix_cameraToClip = matrix;
			++matrix_cameraToClipCheckNumber;
		}
		const Geometry::Matrix4x4f & getMatrix_cameraToClipping() const 				{	return matrix_cameraToClip;	}
		void updateMatrix_cameraToClipping(const RenderingStatus & actual) {
			matrix_cameraToClip = actual.matrix_cameraToClip;
			matrix_cameraToClipCheckNumber = actual.matrix_cameraToClipCheckNumber;
		}
		bool matrix_cameraToClipChanged(const RenderingStatus & actual) const {
			return (matrix_cameraToClipCheckNumber == actual.matrix_cameraToClipCheckNumber) ? false :
					matrix_cameraToClip != actual.matrix_cameraToClip;
		}
	//	@}

	// ------

	//!	@name Texture Units
	//	@{
	private:
		uint32_t textureUnitUsagesCheckNumber;
		std::vector<std::pair<TexUnitUsageParameter,TextureType>> textureUnitParams;

	public:
		void setTextureUnitParams(uint8_t unit, TexUnitUsageParameter use, TextureType t ) {
			++textureUnitUsagesCheckNumber;
			textureUnitParams.at(unit) = std::make_pair(use,t);
		}
		const std::pair<TexUnitUsageParameter,TextureType> & getTextureUnitParams(uint8_t unit) const {
			return textureUnitParams.at(unit);
		}
		bool textureUnitsChanged(const RenderingStatus & actual) const {
			return (textureUnitUsagesCheckNumber == actual.textureUnitUsagesCheckNumber) ? false : 
					textureUnitParams != actual.textureUnitParams;
		}
		void updateTextureUnits(const RenderingStatus & actual) {
			textureUnitParams = actual.textureUnitParams;
			textureUnitUsagesCheckNumber = actual.textureUnitUsagesCheckNumber;
		}

};

}

#endif /* RENDERING_DATA_H_ */

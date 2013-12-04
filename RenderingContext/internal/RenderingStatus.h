/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_DATA_H_
#define RENDERING_DATA_H_

#include "../RenderingParameters.h"
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
		enum type {
			COLOR = 0, LIGHT = 1, MATERIAL = 2, TEXTURE_UNITS = 3, CAMERA = 4, MODELVIEW = 5, PROJECTION = 6, CAMERAINVERSE = 7, MODELVIEWPROJECTION = 8
		};
		static const uint8_t TYPE_COUNT = 9;

		std::vector<uint32_t> checkNumbers;
		Shader * shader;
		bool initialized;

	public:
		explicit RenderingStatus(Shader * _shader = nullptr) : checkNumbers(TYPE_COUNT, 0), shader(_shader), initialized(false),
			lightsEnabled(0), materialEnabled(false),textureUnitUsages(MAX_TEXTURES, TexUnitUsageParameter::GENERAL_PURPOSE) {}
		Shader * getShader() 						{	return shader;	}
		bool isInitialized()const					{	return initialized;	}
		void markInitialized()						{	initialized=true;	}
	//	@}

	// -------------------------------

	//!	@name Camera Matrix
	//	@{
	private:
		Geometry::Matrix4x4f cameraMatrix;
		Geometry::Matrix4x4f cameraInverseMatrix;
	public:
		bool cameraInverseMatrixChanged(const RenderingStatus & actual) const {
			return (checkNumbers[CAMERA] == actual.checkNumbers[CAMERA]) ? false :
					cameraInverseMatrix != actual.cameraInverseMatrix;
		}
		const Geometry::Matrix4x4f & getCameraInverseMatrix() const {
			return cameraInverseMatrix;
		}
		const Geometry::Matrix4x4f & getCameraMatrix() const {
			return cameraMatrix;
		}
		void setCameraInverseMatrix(const Geometry::Matrix4x4f & matrix) {
			cameraInverseMatrix = matrix;
			cameraMatrix = matrix.inverse();
			++checkNumbers[CAMERA];
		}
		void updateCameraMatrix(const RenderingStatus & actual) {
			cameraInverseMatrix = actual.cameraInverseMatrix;
			cameraMatrix = actual.cameraMatrix;
			checkNumbers[CAMERA] = actual.checkNumbers[CAMERA];
		}
	//	@}

	// ------

	//!	@name Lights
	//	@{
	public:
		static const uint8_t MAX_LIGHTS = 8;
	private:
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
			++checkNumbers[LIGHT];
			lights[pos] = light;
			lightsEnabled[pos] = true;
			return pos;
		}
		//! Disable the light with the given number.
		void disableLight(uint8_t lightNumber) {
			assert(lightsEnabled[lightNumber]);
			++checkNumbers[LIGHT];
			lightsEnabled[lightNumber] = false;
		}
		//! Return @c true, if the light with the given light number is enabled.
		bool isLightEnabled(uint8_t lightNumber) const {
			return lightsEnabled[lightNumber];
		}
		bool lightsChanged(const RenderingStatus & actual) const {
			if (checkNumbers[LIGHT] == actual.checkNumbers[LIGHT])
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
			checkNumbers[LIGHT] = actual.checkNumbers[LIGHT];
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
		bool materialEnabled;
		MaterialParameters material;


	public:
		bool isMaterialEnabled()const								{	return materialEnabled;	}
		const MaterialParameters & getMaterialParameters()const	{	return material;	}
		bool materialChanged(const RenderingStatus & actual) const {
			return (checkNumbers[MATERIAL] == actual.checkNumbers[MATERIAL]) ? false :
						(materialEnabled != actual.materialEnabled || material != actual.material);
		}
		void setMaterial(const MaterialParameters & mat) {
			material = mat;
			materialEnabled = true;
			++checkNumbers[MATERIAL];
		}
		void updateMaterial(const RenderingStatus & actual) {
			materialEnabled=actual.materialEnabled;
			material=actual.material;
			checkNumbers[MATERIAL]=actual.checkNumbers[MATERIAL];
		}
		void disableMaterial() {
			materialEnabled = false;
			++checkNumbers[MATERIAL];
		}
	//	@}

	// ------

	//!	@name Modelview Matrix
	//	@{
	private:
		Geometry::Matrix4x4f modelViewMatrix;

	public:
		const Geometry::Matrix4x4f & getModelViewMatrix() const 				{	return modelViewMatrix;	}
		void setModelViewMatrix(const Geometry::Matrix4x4f & matrix) {
			modelViewMatrix = matrix;
			++checkNumbers[MODELVIEW];
		}
		bool modelViewMatrixChanged(const RenderingStatus & actual) const {
			return (checkNumbers[MODELVIEW] == actual.checkNumbers[MODELVIEW]) ? false :
					modelViewMatrix != actual.modelViewMatrix;
		}
		void multModelViewMatrix(const Geometry::Matrix4x4f & matrix) {
			modelViewMatrix *= matrix;
			++checkNumbers[MODELVIEW];
		}
		void updateModelViewMatrix(const RenderingStatus & actual) {
			modelViewMatrix = actual.modelViewMatrix;
			checkNumbers[MODELVIEW] = actual.checkNumbers[MODELVIEW];
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
		Geometry::Matrix4x4f projectionMatrix;
	public:
		void setProjectionMatrix(const Geometry::Matrix4x4f & matrix) {
			projectionMatrix = matrix;
			++checkNumbers[PROJECTION];
		}
		const Geometry::Matrix4x4f & getProjectionMatrix() const 				{	return projectionMatrix;	}
		void updateProjectionMatrix(const RenderingStatus & actual) {
			projectionMatrix = actual.projectionMatrix;
			checkNumbers[PROJECTION] = actual.checkNumbers[PROJECTION];
		}
		bool projectionMatrixChanged(const RenderingStatus & actual) const {
			return (checkNumbers[PROJECTION] == actual.checkNumbers[PROJECTION]) ? false :
					projectionMatrix != actual.projectionMatrix;
		}
	//	@}

	// ------

	//!	@name Texture Units
	//	@{
	public:
		static const uint8_t MAX_TEXTURES = 8;
	private:
		std::vector<TexUnitUsageParameter> textureUnitUsages;

	public:

		void setTextureUnitUsage(uint8_t unit,TexUnitUsageParameter use) {
			++checkNumbers[TEXTURE_UNITS];
			textureUnitUsages.at(unit) = use;
		}
		const std::vector<TexUnitUsageParameter> & getTextureUnitUsages()const	{	
			return textureUnitUsages;
		}
		const TexUnitUsageParameter& getTextureUnitUsage(uint8_t unit)const{	
			return textureUnitUsages.at(unit);
		}
		bool textureUnitsChanged(const RenderingStatus & actual) const {
			if(checkNumbers[TEXTURE_UNITS] == actual.checkNumbers[TEXTURE_UNITS]) 
				return false;
			for(uint_fast8_t i = 0; i < MAX_TEXTURES; ++i) {
				if (textureUnitUsages[i] != actual.textureUnitUsages[i])
					return true;
			}
			return false;
		}
		void updateTextureUnits(const RenderingStatus & actual){
			for(uint_fast8_t i = 0; i < MAX_TEXTURES; ++i)
				textureUnitUsages[i] = actual.textureUnitUsages[i];
			checkNumbers[TEXTURE_UNITS] = actual.checkNumbers[TEXTURE_UNITS];
		}

};

}

#endif /* RENDERING_DATA_H_ */

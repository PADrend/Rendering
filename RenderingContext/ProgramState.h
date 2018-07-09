/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2013 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	Copyright (C) 2018 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_DATA_H_
#define RENDERING_DATA_H_

#include "RenderingParameters.h"
#include "../Texture/TextureType.h"
#include "../BufferView.h"
#include <Geometry/Matrix4x4.h>
#include <bitset>
#include <cassert>
#include <deque>
#include <vector>
#include <cstdint>
#include <cstring>

namespace Rendering {
class UniformRegistry;

//! (internal) Used by shaders and the renderingContext to track the state of shader (and openGL) dependent properties.
class ProgramState {
private:
	Util::Reference<CountedBufferObject> buffer;
public:
	void initBuffers();
	void apply(UniformRegistry* uniformRegistry, const ProgramState & target, bool forced);
	
	// -------------------------------

	//!	@name Camera Matrix
	//	@{
	private:
		uint32_t checkNumber_matrices = 0;
		struct MatrixData {
			Geometry::Matrix4x4f worldToCamera;
			Geometry::Matrix4x4f cameraToWorld;
			Geometry::Matrix4x4f cameraToClipping;
			Geometry::Matrix4x4f clippingToCamera;
			Geometry::Matrix4x4f modelToCamera;
		} matrix;		
		ValueBufferView<MatrixData> matrixBuffer;
	public:
		bool matricesChanged(const ProgramState & actual) const {
			return (checkNumber_matrices == actual.checkNumber_matrices) ? false :
					std::memcmp(&matrix, &actual.matrix, sizeof(MatrixData)) != 0;
		}
		void updateMatrices(const ProgramState & actual) {
			matrix = actual.matrix;
			checkNumber_matrices = actual.checkNumber_matrices;
		}
		const Geometry::Matrix4x4f & getMatrix_cameraToWorld() const { return matrix.cameraToWorld; }
		const Geometry::Matrix4x4f & getMatrix_worldToCamera() const { return matrix.worldToCamera; }
		void setMatrix_cameraToWorld(const Geometry::Matrix4x4f & eyeToWorld) {
			matrix.cameraToWorld = eyeToWorld;
			matrix.worldToCamera = eyeToWorld.inverse();
			++checkNumber_matrices;
		}
		const Geometry::Matrix4x4f & getMatrix_cameraToClipping() const { return matrix.cameraToClipping; }
		void setMatrix_cameraToClipping(const Geometry::Matrix4x4f & mat) {
			matrix.cameraToClipping = mat;
			matrix.clippingToCamera = mat.inverse();
			++checkNumber_matrices;
		}
		const Geometry::Matrix4x4f & getMatrix_modelToCamera() const { return matrix.modelToCamera; }
		void setMatrix_modelToCamera(const Geometry::Matrix4x4f & mat) {
			matrix.modelToCamera = mat;
			++checkNumber_matrices;
		}
		void multModelViewMatrix(const Geometry::Matrix4x4f & mat) {
			matrix.modelToCamera *= mat;
			++checkNumber_matrices;
		}
	//	@}

	// ------

	//!	@name Materials
	//	@{
	private:
		uint32_t materialCheckNumber = 0;
		MaterialParameters material = false;
		ValueBufferView<MaterialParameters> materialBuffer;
	public:
		bool isMaterialEnabled() const { return material.isEnabled(); }
		const MaterialParameters & getMaterialParameters()const	{	return material;	}
		bool materialChanged(const ProgramState & actual) const {
			return (materialCheckNumber == actual.materialCheckNumber) ? false : material != actual.material;
		}
		void setMaterial(const MaterialParameters & mat) {
			material = mat;
			material.setEnabled(true);
			++materialCheckNumber;
		}
		void updateMaterial(const ProgramState & actual) {
			material=actual.material;
			materialCheckNumber=actual.materialCheckNumber;
		}
		void disableMaterial() {
			material.setEnabled(false);
			++materialCheckNumber;
		}
	//	@}

	// ------

	//!	@name Lights
	//	@{
	public:
		static const uint8_t MAX_LIGHTS = 8;
	private:
		uint32_t lightsCheckNumber = 0;
		//! Storage of light parameters.
		LightParameters lights[MAX_LIGHTS];
		StructuredBufferView<LightParameters> lightBuffer;

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
		bool lightsChanged(const ProgramState & actual) const {
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
		void updateLights(const ProgramState & actual) {
			lightsEnabled = actual.lightsEnabled;
			lightsCheckNumber = actual.lightsCheckNumber;
		}
		void updateLightParameter(uint8_t lightNumber, const LightParameters & light) {
			assert(lightNumber < MAX_LIGHTS);
			lights[lightNumber] = light;
		}
	//	@}

	// ------

	//!	@name Point
	//	@{
	private:
		PointParameters pointParameters;
	public:
		bool pointParametersChanged(const ProgramState & actual) const {
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

	//!	@name Texture Units
	//	@{
	private:
		uint32_t textureUnitUsagesCheckNumber = 0;
		typedef std::vector<std::pair<TexUnitUsageParameter,TextureType>> TexUnitTypeVec_t;
		TexUnitTypeVec_t textureUnitParams = TexUnitTypeVec_t(MAX_TEXTURES,{TexUnitUsageParameter::DISABLED,TextureType::TEXTURE_2D});

	public:
		void setTextureUnitParams(uint8_t unit, TexUnitUsageParameter use, TextureType t ) {
			++textureUnitUsagesCheckNumber;
			textureUnitParams.at(unit) = std::make_pair(use,t);
		}
		const std::pair<TexUnitUsageParameter,TextureType> & getTextureUnitParams(uint8_t unit) const {
			return textureUnitParams.at(unit);
		}
		bool textureUnitsChanged(const ProgramState & actual) const {
			return (textureUnitUsagesCheckNumber == actual.textureUnitUsagesCheckNumber) ? false : 
					textureUnitParams != actual.textureUnitParams;
		}
		void updateTextureUnits(const ProgramState & actual) {
			textureUnitParams = actual.textureUnitParams;
			textureUnitUsagesCheckNumber = actual.textureUnitUsagesCheckNumber;
		}

};

}

#endif /* RENDERING_DATA_H_ */

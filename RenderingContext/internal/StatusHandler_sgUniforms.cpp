/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2013 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2013 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "StatusHandler_sgUniforms.h"
#include "RenderingStatus.h"
#include "../../Shader/Shader.h"
#include "../../Shader/UniformRegistry.h"

namespace Rendering {
namespace StatusHandler_sgUniforms{

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

static const Uniform::UniformName UNIFORM_SG_MATRIX_MODEL_TO_CAMERA("sg_matrix_modelToCamera");
static const Uniform::UniformName UNIFORM_SG_MATRIX_MODEL_TO_CAMERA_OLD("sg_modelViewMatrix");
static const Uniform::UniformName UNIFORM_SG_MATRIX_CAMERA_TO_CLIPPING("sg_matrix_cameraToClipping");
static const Uniform::UniformName UNIFORM_SG_MATRIX_CAMERA_TO_CLIPPING_OLD("sg_projectionMatrix");
static const Uniform::UniformName UNIFORM_SG_MATRIX_MODEL_TO_CLIPPING("sg_matrix_modelToClipping");
static const Uniform::UniformName UNIFORM_SG_MATRIX_MODEL_TO_CLIPPING_OLD("sg_modelViewProjectionMatrix");
static const Uniform::UniformName UNIFORM_SG_MATRIX_WORLD_TO_CAMERA("sg_matrix_worldToCamera");
static const Uniform::UniformName UNIFORM_SG_MATRIX_WORLD_TO_CAMERA_OLD("sg_cameraMatrix");
static const Uniform::UniformName UNIFORM_SG_MATRIX_CAMERA_TO_WORLD("sg_matrix_cameraToWorld");
static const Uniform::UniformName UNIFORM_SG_MATRIX_CAMERA_TO_WORLD_OLD("sg_cameraInverseMatrix");

static const Uniform::UniformName UNIFORM_SG_LIGHT_COUNT("sg_lightCount");
static const Uniform::UniformName UNIFORM_SG_POINT_SIZE("sg_pointSize");

static const UniformNameArray_t UNIFORM_SG_LIGHT_SOURCES_POSITION(createNames("sg_LightSource[", RenderingStatus::MAX_LIGHTS, "].position"));
static const UniformNameArray_t UNIFORM_SG_LIGHT_SOURCES_DIRECTION(createNames("sg_LightSource[", RenderingStatus::MAX_LIGHTS, "].direction"));
static const UniformNameArray_t UNIFORM_SG_LIGHT_SOURCES_TYPE(createNames("sg_LightSource[", RenderingStatus::MAX_LIGHTS, "].type"));
static const UniformNameArray_t UNIFORM_SG_LIGHT_SOURCES_CONSTANT(createNames("sg_LightSource[", RenderingStatus::MAX_LIGHTS, "].constant"));
static const UniformNameArray_t UNIFORM_SG_LIGHT_SOURCES_LINEAR(createNames("sg_LightSource[", RenderingStatus::MAX_LIGHTS, "].linear"));
static const UniformNameArray_t UNIFORM_SG_LIGHT_SOURCES_QUADRATIC(createNames("sg_LightSource[", RenderingStatus::MAX_LIGHTS, "].quadratic"));
static const UniformNameArray_t UNIFORM_SG_LIGHT_SOURCES_AMBIENT(createNames("sg_LightSource[", RenderingStatus::MAX_LIGHTS, "].ambient"));
static const UniformNameArray_t UNIFORM_SG_LIGHT_SOURCES_DIFFUSE(createNames("sg_LightSource[", RenderingStatus::MAX_LIGHTS, "].diffuse"));
static const UniformNameArray_t UNIFORM_SG_LIGHT_SOURCES_SPECULAR(createNames("sg_LightSource[", RenderingStatus::MAX_LIGHTS, "].specular"));
static const UniformNameArray_t UNIFORM_SG_LIGHT_SOURCES_EXPONENT(createNames("sg_LightSource[", RenderingStatus::MAX_LIGHTS, "].exponent"));
static const UniformNameArray_t UNIFORM_SG_LIGHT_SOURCES_COSCUTOFF(createNames("sg_LightSource[", RenderingStatus::MAX_LIGHTS, "].cosCutoff"));

static const Uniform::UniformName UNIFORM_SG_TEXTURE_ENABLED("sg_textureEnabled");
static const UniformNameArray_t UNIFORM_SG_TEXTURES(createNames("sg_texture", MAX_TEXTURES, ""));
static const Uniform::UniformName UNIFORM_SG_USE_MATERIALS("sg_useMaterials");
static const Uniform::UniformName UNIFORM_SG_MATERIAL_AMBIENT("sg_Material.ambient");
static const Uniform::UniformName UNIFORM_SG_MATERIAL_DIFFUSE("sg_Material.diffuse");
static const Uniform::UniformName UNIFORM_SG_MATERIAL_SPECULAR("sg_Material.specular");
static const Uniform::UniformName UNIFORM_SG_MATERIAL_SHININESS("sg_Material.shininess");

void apply(RenderingStatus & target, const RenderingStatus & actual, bool forced){

	Shader * shader = target.getShader();
	std::deque<Uniform> uniforms;

	// camera  & inverse
	bool cc = false;
	if (forced || target.matrixCameraToWorldChanged(actual)) {
		cc = true;
		target.updateMatrix_cameraToWorld(actual);

		uniforms.emplace_back(UNIFORM_SG_MATRIX_WORLD_TO_CAMERA, actual.getMatrix_worldToCamera());
		uniforms.emplace_back(UNIFORM_SG_MATRIX_CAMERA_TO_WORLD, actual.getMatrix_cameraToWorld());
		
		uniforms.emplace_back(UNIFORM_SG_MATRIX_WORLD_TO_CAMERA_OLD, actual.getMatrix_worldToCamera());
		uniforms.emplace_back(UNIFORM_SG_MATRIX_CAMERA_TO_WORLD_OLD, actual.getMatrix_cameraToWorld());
	}

	// lights
	if (forced || cc || target.lightsChanged(actual)) {

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
			for (uint_fast8_t i = numEnabledLights; i < RenderingStatus::MAX_LIGHTS; ++i) {
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
	}

	// materials
	if (forced || target.materialChanged(actual)) {
		target.updateMaterial(actual);

		uniforms.emplace_back(UNIFORM_SG_USE_MATERIALS, actual.isMaterialEnabled());
		if (forced || actual.isMaterialEnabled()) {
			const MaterialParameters & material = actual.getMaterialParameters();
			uniforms.emplace_back(UNIFORM_SG_MATERIAL_AMBIENT, material.getAmbient());
			uniforms.emplace_back(UNIFORM_SG_MATERIAL_DIFFUSE, material.getDiffuse());
			uniforms.emplace_back(UNIFORM_SG_MATERIAL_SPECULAR, material.getSpecular());
			uniforms.emplace_back(UNIFORM_SG_MATERIAL_SHININESS, material.getShininess());
		}
	}

	// modelview & projection
	{
		bool pc = false;
		bool mc = false;

		if (forced || target.matrix_modelToCameraChanged(actual)) {
			mc = true;
			target.updateModelViewMatrix(actual);
			uniforms.emplace_back(UNIFORM_SG_MATRIX_MODEL_TO_CAMERA, actual.getMatrix_modelToCamera());
			uniforms.emplace_back(UNIFORM_SG_MATRIX_MODEL_TO_CAMERA_OLD, actual.getMatrix_modelToCamera());
		}

		if (forced || target.matrix_cameraToClipChanged(actual)) {
			pc = true;
			target.updateMatrix_cameraToClipping(actual);
			uniforms.emplace_back(UNIFORM_SG_MATRIX_CAMERA_TO_CLIPPING, actual.getMatrix_cameraToClipping());
			uniforms.emplace_back(UNIFORM_SG_MATRIX_CAMERA_TO_CLIPPING_OLD, actual.getMatrix_cameraToClipping());
		}
		if (forced || pc || mc) {
			const auto m = actual.getMatrix_cameraToClipping() * actual.getMatrix_modelToCamera();
			uniforms.emplace_back(UNIFORM_SG_MATRIX_MODEL_TO_CLIPPING, m);
			uniforms.emplace_back(UNIFORM_SG_MATRIX_MODEL_TO_CLIPPING_OLD, m);
		}
	}

	// Point
	if(forced || target.pointParametersChanged(actual)) {
		target.setPointParameters(actual.getPointParameters());
		uniforms.emplace_back(UNIFORM_SG_POINT_SIZE, actual.getPointParameters().getSize());
	}

	// TEXTURE UNITS
	if (forced || target.textureUnitsChanged(actual)) {
		std::deque<bool> textureUnitsUsedForRendering;
		for(uint_fast8_t unit = 0; unit < MAX_TEXTURES; ++unit) {
			const TexUnitUsageParameter usage = actual.getTextureUnitParams(unit).first;
			textureUnitsUsedForRendering.emplace_back(usage != TexUnitUsageParameter::GENERAL_PURPOSE && usage!=TexUnitUsageParameter::DISABLED);

			// for each shader, this is only necessary once...
			uniforms.emplace_back(UNIFORM_SG_TEXTURES[unit], static_cast<int32_t>(unit));
		}
		uniforms.emplace_back(UNIFORM_SG_TEXTURE_ENABLED, textureUnitsUsedForRendering);
		target.updateTextureUnits(actual);
	}

	for(const auto & uniform : uniforms) {
		shader->_getUniformRegistry()->setUniform(uniform, false, forced);
	}
}

}
}

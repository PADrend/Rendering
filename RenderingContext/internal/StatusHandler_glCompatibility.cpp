/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2013 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2013 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "StatusHandler_glCompatibility.h"
#include "RenderingStatus.h"
#include "../../GLHeader.h"
#include "../../Helper.h"

#ifdef WIN32
#include <GL/wglew.h>
#endif

namespace Rendering {
namespace StatusHandler_glCompatibility{

void apply(RenderingStatus & target, const RenderingStatus & actual, bool forced) {
#ifdef LIB_GL

	const bool cc = target.matrixCameraToWorldChanged(actual);
	if (forced || cc) {
		target.updateMatrix_cameraToWorld(actual);
	}

	if (forced || target.matrix_cameraToClipChanged(actual)) {
		glMatrixMode(GL_PROJECTION);
		glLoadTransposeMatrixf(actual.getMatrix_cameraToClipping().getData());
		glMatrixMode(GL_MODELVIEW);
		target.updateMatrix_cameraToClipping(actual);
	}

	if (forced || target.matrix_modelToCameraChanged(actual)) {
		glLoadTransposeMatrixf(actual.getMatrix_modelToCamera().getData());
		target.updateModelViewMatrix(actual);
	}

	if (forced || target.materialChanged(actual)) {
		static const float ambient[4] = {0.2f, 0.2f, 0.2f, 1.0f};
		static const float diffuse[4] = {0.8f, 0.8f, 0.8f, 1.0f};
		static const float specular[4] = {0.0f, 0.0f, 0.0f, 1.0f};

		const MaterialParameters & materialParams = actual.getMaterialParameters();

		if (actual.isMaterialEnabled()) {
			if (materialParams.getColorMaterial()) {
				glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
				glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
				glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
				glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);
				glEnable(GL_COLOR_MATERIAL);
				glColor4fv(materialParams.getDiffuse().data());
			} else {
				glDisable(GL_COLOR_MATERIAL);
				glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, materialParams.getAmbient().data());
				glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, materialParams.getDiffuse().data());
				glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, materialParams.getSpecular().data());
				glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, materialParams.getShininess());
			}
		} else {
			glEnable(GL_COLOR_MATERIAL);
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
			glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
			glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
			glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);
		}
		target.updateMaterial(actual);
	}

	//lights
	const bool lc = target.lightsChanged(actual);
	if (lc || cc || forced) {

		const uint_fast8_t numEnabledLights = actual.getNumEnabledLights();

		target.updateLights(actual);

		for (uint_fast8_t i = 0; i < numEnabledLights; ++i)
		glEnable(GL_LIGHT0 + static_cast<GLenum>(i));

		for (uint_fast8_t i = numEnabledLights; i < RenderingStatus::MAX_LIGHTS; ++i)
		glDisable(GL_LIGHT0 + static_cast<GLenum>(i));

		glPushMatrix();
		glLoadTransposeMatrixf(actual.getMatrix_worldToCamera().getData());

		for (uint_fast8_t i = 0; i < numEnabledLights; ++i) {

			const GLenum lightNumber = GL_LIGHT0 + static_cast<GLenum>(i);
			const LightParameters & parameters = actual.getEnabledLight(i);

			glLightfv(lightNumber, GL_AMBIENT, parameters.ambient.data());
			glLightfv(lightNumber, GL_DIFFUSE, parameters.diffuse.data());
			glLightfv(lightNumber, GL_SPECULAR, parameters.specular.data());

			const Geometry::Vec4 direction( parameters.direction, 0.0);
			
			if (parameters.type == LightParameters::DIRECTIONAL) {
				glLightfv(lightNumber, GL_POSITION, (-direction).getVec());
				glLightf(lightNumber, GL_CONSTANT_ATTENUATION, 1.0f);
				glLightf(lightNumber, GL_LINEAR_ATTENUATION, 0.0f);
				glLightf(lightNumber, GL_QUADRATIC_ATTENUATION, 0.0f);
			} else {
				const Geometry::Vec4 position( parameters.position, 1.0);
				glLightfv(lightNumber, GL_POSITION, position.getVec());
				glLightf(lightNumber, GL_CONSTANT_ATTENUATION, parameters.constant);
				glLightf(lightNumber, GL_LINEAR_ATTENUATION, parameters.linear);
				glLightf(lightNumber, GL_QUADRATIC_ATTENUATION, parameters.quadratic);
			}

			if (parameters.type == LightParameters::SPOT) {
				glLightf(lightNumber, GL_SPOT_CUTOFF, parameters.cutoff);
				glLightfv(lightNumber, GL_SPOT_DIRECTION, direction.getVec());
				glLightf(lightNumber, GL_SPOT_EXPONENT, parameters.exponent);
			} else {
				glLightf(lightNumber, GL_SPOT_CUTOFF, 180.0f);
				glLightfv(lightNumber, GL_SPOT_DIRECTION, Geometry::Vec4f(0.0f, 0.0f, -1.0f, 0.0f).getVec());
				glLightf(lightNumber, GL_SPOT_EXPONENT, 0.0f);
			}
			target.updateLightParameter(i,parameters);
		}
		glPopMatrix();
	}

	// Point
	if(forced || target.pointParametersChanged(actual)) {
		glPointSize(actual.getPointParameters().getSize());
		if(actual.getPointParameters().isPointSmoothingEnabled()){
			glEnable(GL_POINT_SMOOTH);
		}else{
			glDisable(GL_POINT_SMOOTH);
		}
		target.setPointParameters(actual.getPointParameters());
	}
	GET_GL_ERROR();

	// Texturing
	if (forced || target.textureUnitsChanged(actual)) {
		for(uint_fast8_t unit = 0; unit < MAX_TEXTURES; ++unit) {
			// enable/disable the fixed-function pipeline texture processing
			const auto & params = actual.getTextureUnitParams(unit);
			const auto & oldParams = target.getTextureUnitParams(unit);
			if(!forced && params==oldParams)
				continue;
				
			glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(unit));

			switch(params.second){
				case TextureType::TEXTURE_2D:
				case TextureType::TEXTURE_2D_ARRAY:
				case TextureType::TEXTURE_CUBE_MAP:
				case TextureType::TEXTURE_CUBE_MAP_ARRAY:
					if(params.first == TexUnitUsageParameter::TEXTURE_MAPPING) 
						glEnable(GL_TEXTURE_2D);
					else
						glDisable(GL_TEXTURE_2D);
					break;
				case TextureType::TEXTURE_1D:
				case TextureType::TEXTURE_1D_ARRAY:
				case TextureType::TEXTURE_BUFFER:
					if(params.first == TexUnitUsageParameter::TEXTURE_MAPPING) 
						glEnable(GL_TEXTURE_1D);
					else
						glDisable(GL_TEXTURE_1D);
					break;
				case TextureType::TEXTURE_3D:
					if(params.first == TexUnitUsageParameter::TEXTURE_MAPPING) 
						glEnable(GL_TEXTURE_3D);
					else
						glDisable(GL_TEXTURE_3D);
					break;
				default:
					glDisable(GL_TEXTURE_2D);
			}
			
		}
		glActiveTexture(GL_TEXTURE0);
	}
	GET_GL_ERROR();
#endif /* LIB_GL */
}

}

}

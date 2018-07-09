/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2013 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2013 Ralf Petring <ralf@petring.net>
	Copyright (C) 2018 Sascha Brandt <sascha@brandt.graphics>

	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "ProgramState.h"
#include "../Shader/Shader.h"
#include "../Shader/UniformRegistry.h"
#include "../GLHeader.h"

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

static const Uniform::UniformName UNIFORM_SG_LIGHT_COUNT("sg_lightCount");
static const Uniform::UniformName UNIFORM_SG_POINT_SIZE("sg_pointSize");
static const Uniform::UniformName UNIFORM_SG_TEXTURE_ENABLED("sg_textureEnabled");
static const UniformNameArray_t UNIFORM_SG_TEXTURES(createNames("sg_texture", MAX_TEXTURES, ""));

inline uint32_t align(uint32_t offset, uint32_t alignment) {
  return alignment > 1 ? (offset + (alignment - offset % alignment) % alignment) : offset;
}

void ProgramState::initBuffers() {
	buffer = new CountedBufferObject;
	int32_t alignment;
	glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &alignment);
	
	uint32_t matrixOffset = 0;
	uint32_t materialOffset = align(matrixOffset + sizeof(MatrixData), alignment);
	uint32_t lightOffset = align(materialOffset + sizeof(MaterialParameters), alignment);
	uint32_t totalSize = align(lightOffset + sizeof(LightParameters) * MAX_LIGHTS, alignment);
	buffer->get().allocate(totalSize, BufferObject::FLAG_DYNAMIC_STORAGE);
	
	matrixBuffer.relocate(buffer.get(), matrixOffset);
	matrixBuffer.allocate();
	matrixBuffer.upload(matrix);
	matrixBuffer.bind(GL_UNIFORM_BUFFER, 0);
	
	materialBuffer.relocate(buffer.get(), materialOffset);
	materialBuffer.allocate();
	materialBuffer.upload(material);
	materialBuffer.bind(GL_UNIFORM_BUFFER, 1);
	
	lightBuffer.relocate(buffer.get(), lightOffset);
	lightBuffer.allocate(MAX_LIGHTS);
	lightBuffer.upload(&lights[0]);
	lightBuffer.bind(GL_UNIFORM_BUFFER, 2);
}

void ProgramState::apply(UniformRegistry* uniformRegistry, const ProgramState & target, bool forced) {
	std::deque<Uniform> uniforms;

	// matrices
	bool cc = false;
	if (forced || matricesChanged(target)) {
		cc = true;
		updateMatrices(target);
		matrixBuffer.upload(matrix);
	}

	// materials
	if (forced || materialChanged(target)) {
		updateMaterial(target);
		materialBuffer.upload(material);
	}

	// lights
	if (forced || cc || lightsChanged(target)) {
		updateLights(target);
		for(uint_fast8_t i=0; i<MAX_LIGHTS; ++i)
			updateLightParameter(i, target.lights[i]);
		uniforms.emplace_back(UNIFORM_SG_LIGHT_COUNT, static_cast<int> (target.getNumEnabledLights()));
		lightBuffer.upload(&lights[0]);
	}

	// Point
	if(forced || pointParametersChanged(target)) {
		setPointParameters(target.getPointParameters());
		uniforms.emplace_back(UNIFORM_SG_POINT_SIZE, target.getPointParameters().getSize());
	}

	// TEXTURE UNITS
	if (forced || textureUnitsChanged(target)) {
		std::deque<bool> textureUnitsUsedForRendering;
		for(uint_fast8_t unit = 0; unit < MAX_TEXTURES; ++unit) {
			const TexUnitUsageParameter usage = target.getTextureUnitParams(unit).first;
			textureUnitsUsedForRendering.emplace_back(usage != TexUnitUsageParameter::GENERAL_PURPOSE && usage!=TexUnitUsageParameter::DISABLED);

			// for each shader, this is only necessary once...
			uniforms.emplace_back(UNIFORM_SG_TEXTURES[unit], static_cast<int32_t>(unit));
		}
		uniforms.emplace_back(UNIFORM_SG_TEXTURE_ENABLED, textureUnitsUsedForRendering);
		updateTextureUnits(target);
	}

	for(const auto & uniform : uniforms) {
		uniformRegistry->setUniform(uniform, false, forced);
	}
}

}

/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "ShaderUtils.h"
#include "Shader.h"
#include "../State/ShaderLayout.h"
#include <Util/References.h>
#include <Util/StringUtils.h>
#include <Util/TypeConstant.h>
#include <string>

#include <spirv_cross.hpp>
#include <vulkan/vulkan.hpp>

namespace Rendering {

//-------------

namespace ShaderUtils {

//-------------

static Uniform::dataType_t getUniformType(const spirv_cross::SPIRType& type) {
	switch(type.basetype) {
		case spirv_cross::SPIRType::Boolean:
			switch(type.vecsize) {
				case 1: return Uniform::UNIFORM_BOOL;
				case 2: return Uniform::UNIFORM_VEC2B;
				case 3: return Uniform::UNIFORM_VEC3B;
				case 4: return Uniform::UNIFORM_VEC4B;
				default: return Uniform::UNIFORM_BOOL;
			}
		case spirv_cross::SPIRType::Int:
			switch(type.vecsize) {
				case 1: return Uniform::UNIFORM_INT;
				case 2: return Uniform::UNIFORM_VEC2I;
				case 3: return Uniform::UNIFORM_VEC3I;
				case 4: return Uniform::UNIFORM_VEC4I;
				default: return Uniform::UNIFORM_INT;
			}
		//case spirv_cross::SPIRType::UInt: return Uniform::UNIFORM_UINT;
		case spirv_cross::SPIRType::Float:
			switch(type.vecsize) {
				case 1: return Uniform::UNIFORM_FLOAT;
				case 2: return type.columns == 2 ? Uniform::UNIFORM_MATRIX_2X2F : Uniform::UNIFORM_VEC2F;
				case 3: return type.columns == 3 ? Uniform::UNIFORM_MATRIX_3X3F : Uniform::UNIFORM_VEC3F;
				case 4: return type.columns == 4 ? Uniform::UNIFORM_MATRIX_4X4F : Uniform::UNIFORM_VEC4F;
				default: return Uniform::UNIFORM_FLOAT;
			}			
		default: return Uniform::UNIFORM_INT;
	}
}

//-------------

static std::vector<ShaderResourceMember> getResourceMembers(spirv_cross::Compiler& compiler, const spirv_cross::SPIRType& type) {
	std::vector<ShaderResourceMember> result;
	uint32_t member_count = type.member_types.size();

	for (uint32_t i = 0; i < member_count; ++i) {
		auto &member_type = compiler.get_type(type.member_types[i]);
		ShaderResourceMember m;
		m.name = compiler.get_member_name(type.self, i);
		m.offset = compiler.type_struct_member_offset(type, i);
		m.count = 1;
		m.type = getUniformType(member_type);
		uint32_t array_stride = 0;

		if(!member_type.array.empty()) {
			array_stride = compiler.type_struct_member_array_stride(type, i);
			m.count = member_type.array[0];
		}

		if(member_type.basetype == spirv_cross::SPIRType::Struct) {
			auto struct_members = getResourceMembers(compiler, member_type);
			std::vector<ShaderResourceMember> tmp;

			if(m.count > 1) {
				for(uint32_t j=0; j<m.count; ++j) {
					for(auto& sm : struct_members) {
						tmp.emplace_back(m.name.getString() + "[" + Util::StringUtils::toString(j) + "]." + sm.name.getString(), m.offset + j*array_stride + sm.offset, sm.count, sm.type);
					}
				}
			} else {
				for(auto& sm : struct_members) {
					tmp.emplace_back(m.name.getString() + "." + sm.name.getString(), m.offset + sm.offset, sm.count, sm.type);
				}
			}
			
			std::move(tmp.begin(), tmp.end(), std::back_inserter(result));
		} else {
			result.emplace_back(std::move(m));
		}		
	}
	return result;
}

//-------------

static ShaderResource readPushConstant(spirv_cross::Compiler& compiler, spirv_cross::Resource& resource, ShaderStage stage) {
	ShaderResource result{resource.name, 0, 0, {ShaderResourceType::PushConstant, stage}};
	const auto& spirvType = compiler.get_type_from_variable(resource.id);
	result.size = compiler.get_declared_struct_size_runtime_array(spirvType, 0); // TODO: specify runtime array size
	result.offset = std::numeric_limits<std::uint32_t>::max();
	for(auto i=0u; i < spirvType.member_types.size(); ++i) 
		result.offset = std::min(result.offset, compiler.get_member_decoration(spirvType.self, i, spv::DecorationOffset));
	result.size -= result.offset;

	result.members = getResourceMembers(compiler, compiler.get_type(resource.base_type_id)); // recursively get members of structs

	return result;
}

//-------------

static ShaderResource readSpecializationConstant(spirv_cross::Compiler& compiler, spirv_cross::SpecializationConstant& resource, ShaderStage stage) {
	ShaderResource result{compiler.get_name(resource.id), 0, 0, {ShaderResourceType::SpecializationConstant, stage}};
	const auto& spirvValue = compiler.get_constant(resource.id);
	const auto& spirvType = compiler.get_type(spirvValue.constant_type);
	switch (spirvType.basetype) {
		case spirv_cross::SPIRType::BaseType::Boolean:
		case spirv_cross::SPIRType::BaseType::Char:
		case spirv_cross::SPIRType::BaseType::Int:
		case spirv_cross::SPIRType::BaseType::UInt:
		case spirv_cross::SPIRType::BaseType::Float:
			result.size = 4;
			break;
		case spirv_cross::SPIRType::BaseType::Int64:
		case spirv_cross::SPIRType::BaseType::UInt64:
		case spirv_cross::SPIRType::BaseType::Double:
			result.size = 8;
			break;
		default:
			result.size = 0;
			break;
	}
	result.offset = 0;
	result.constantId = resource.constant_id;
	return result;
}

//-------------

static ShaderResource readShaderResource(spirv_cross::Compiler& compiler, spirv_cross::Resource& resource, ShaderStage stage, ShaderResourceType type) {
	ShaderResource result{resource.name, 0, 0, {type, stage}};
	const auto& spirvType = compiler.get_type_from_variable(resource.id);
	result.layout.elementCount = spirvType.array.size() ? spirvType.array[0] : 1;
	result.vecSize = spirvType.vecsize;

	switch(type) {
		case ShaderResourceType::BufferUniform:
		case ShaderResourceType::BufferStorage:
			result.size = compiler.get_declared_struct_size_runtime_array(spirvType, 0); // TODO: specify runtime array size
			result.members = getResourceMembers(compiler, compiler.get_type(resource.base_type_id)); // recursively get members of structs
			break;
		default: break;
	}
	
	result.location = compiler.get_decoration(resource.id, spv::DecorationLocation);
	result.set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
	result.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
	result.inputAttachmentIndex = compiler.get_decoration(resource.id, spv::DecorationInputAttachmentIndex);
	result.columns = spirvType.columns;
	return result;
}

//-------------

ShaderResourceList reflect(ShaderStage stage, const std::vector<uint32_t>& code) {
	ShaderResourceList resources;
	spirv_cross::Compiler compiler(code);	
	auto spvResources = compiler.get_shader_resources();
	for(auto& res : spvResources.stage_inputs)
		resources.emplace_back(readShaderResource(compiler, res, stage, ShaderResourceType::Input));
	for(auto& res : spvResources.subpass_inputs)
		resources.emplace_back(readShaderResource(compiler, res, stage, ShaderResourceType::InputAttachment));
	for(auto& res : spvResources.stage_outputs)
		resources.emplace_back(readShaderResource(compiler, res, stage, ShaderResourceType::Output));
	for(auto& res : spvResources.separate_images)
		resources.emplace_back(readShaderResource(compiler, res, stage, ShaderResourceType::Image));
	for(auto& res : spvResources.sampled_images)
		resources.emplace_back(readShaderResource(compiler, res, stage, ShaderResourceType::ImageSampler));
	for(auto& res : spvResources.storage_images)
		resources.emplace_back(readShaderResource(compiler, res, stage, ShaderResourceType::ImageStorage));
	for(auto& res : spvResources.separate_samplers)
		resources.emplace_back(readShaderResource(compiler, res, stage, ShaderResourceType::Sampler));
	for(auto& res : spvResources.uniform_buffers)
		resources.emplace_back(readShaderResource(compiler, res, stage, ShaderResourceType::BufferUniform));
	for(auto& res : spvResources.storage_buffers)
		resources.emplace_back(readShaderResource(compiler, res, stage, ShaderResourceType::BufferStorage));
	for(auto& res : spvResources.push_constant_buffers)
		resources.emplace_back(readPushConstant(compiler, res, stage));
	for(auto& res : compiler.get_specialization_constants())
		resources.emplace_back(readSpecializationConstant(compiler, res, stage));
	
	return resources;
}

//-------------

ShaderRef createDefaultShader(const DeviceRef& device) {
	const std::string vertexShader = R"vs(
		#version 450
		layout(location = 0) in vec3 sg_Position;
		layout(location = 1) in vec4 sg_Color;
		layout(location = 0) out vec4 fragColor;
		void main() {
			gl_Position = vec4(sg_Position, 1.0);
			fragColor = sg_Color;
		}
	)vs";

	const std::string fragmentShader = R"fs(
		#version 450
		layout(location = 0) in vec4 fragColor;
		layout(location = 0) out vec4 outColor;
		void main() {
			outColor = fragColor;
		}
	)fs";
	return Shader::createShader(device, vertexShader, fragmentShader);
}

//-------------

ShaderRef createNormalToColorShader() {
	const std::string vertexProgram(
R"***(#version 110
uniform mat4 sg_matrix_cameraToWorld;
uniform mat4 sg_matrix_modelToCamera;
varying vec3 normal;

void main() {
	normal = normalize((sg_matrix_cameraToWorld * sg_matrix_modelToCamera * vec4(gl_Normal, 0.0)).xyz);
	gl_Position = ftransform();
}
)***");
	const std::string fragmentProgram(
R"***(#version 110
varying vec3 normal;

void main() {
	gl_FragColor = vec4(0.5 * normalize(normal) + 0.5, 1.0);
}
)***");
	return Shader::createShader(vertexProgram, fragmentProgram, Shader::USE_GL | Shader::USE_UNIFORMS);
}

//-------------

}
}

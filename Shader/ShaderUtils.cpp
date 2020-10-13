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
#include "../Core/Common.h"
#include <Util/References.h>
#include <string>

#include <spirv_cross.hpp>
#include <vulkan/vulkan.hpp>

namespace Rendering {
namespace ShaderUtils {

//-------------

std::string toString(ShaderStage stage) {
	switch(stage) {
		case ShaderStage::Undefined: return "Undefined";
		case ShaderStage::Vertex: return "Vertex";
		case ShaderStage::TessellationControl: return "TessellationControl";
		case ShaderStage::TessellationEvaluation: return "TessellationEvaluation";
		case ShaderStage::Geometry: return "Geometry";
		case ShaderStage::Fragment: return "Fragment";
		case ShaderStage::Compute: return "Compute";
	}
	return "";
}

//-------------

std::string toString(ShaderResourceType type) {
	switch(type) {
		case ShaderResourceType::Input: return "Input";
		case ShaderResourceType::InputAttachment: return "InputAttachment";
		case ShaderResourceType::Output: return "Output";
		case ShaderResourceType::Image: return "Image";
		case ShaderResourceType::ImageSampler: return "ImageSampler";
		case ShaderResourceType::ImageStorage: return "ImageStorage";
		case ShaderResourceType::Sampler: return "Sampler";
		case ShaderResourceType::BufferUniform: return "BufferUniform";
		case ShaderResourceType::BufferStorage: return "BufferStorage";
		case ShaderResourceType::PushConstant: return "PushConstant";
		case ShaderResourceType::SpecializationConstant: return "SpecializationConstant";
	}
	return "";
}

//-------------

std::string toString(const ShaderResource& resource) {
	return "ShaderResource(name " + resource.name + ", " 
		+ "stage " + toString(resource.stages) + ", "
		+ "type " + toString(resource.type) + ", "
		+ "set " + std::to_string(resource.set) + ", "
		+ "binding " + std::to_string(resource.binding) + ", "
		+ "location " + std::to_string(resource.location) + ", "
		+ "input_attachment_index " + std::to_string(resource.input_attachment_index) + ", "
		+ "vec_size " + std::to_string(resource.vec_size) + ", "
		+ "columns " + std::to_string(resource.columns) + ", "
		+ "array_size " + std::to_string(resource.array_size) + ", "
		+ "offset " + std::to_string(resource.offset) + ", "
		+ "size " + std::to_string(resource.size) + ", "
		+ "constant_id " + std::to_string(resource.constant_id) + ", "
		+ "dynamic " + std::to_string(resource.dynamic) + ")";
}

//-------------

static ShaderResource readPushConstant(spirv_cross::Compiler& compiler, spirv_cross::Resource& resource, ShaderStage stage) {
	ShaderResource result{resource.name, stage, ShaderResourceType::PushConstant};
	const auto& spirvType = compiler.get_type_from_variable(resource.id);
	result.size = compiler.get_declared_struct_size_runtime_array(spirvType, 0); // TODO: specify runtime array size
	result.offset = std::numeric_limits<std::uint32_t>::max();
	for(auto i=0u; i < spirvType.member_types.size(); ++i) 
		result.offset = std::min(result.offset, compiler.get_member_decoration(spirvType.self, i, spv::DecorationOffset));
	result.size -= result.offset;
	return result;
}

//-------------

static ShaderResource readSpecializationConstant(spirv_cross::Compiler& compiler, spirv_cross::SpecializationConstant& resource, ShaderStage stage) {
	ShaderResource result{compiler.get_name(resource.id), stage, ShaderResourceType::SpecializationConstant};
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
	result.constant_id = resource.constant_id;
	return result;
}

//-------------

static ShaderResource readShaderResource(spirv_cross::Compiler& compiler, spirv_cross::Resource& resource, ShaderStage stage, ShaderResourceType type) {
	ShaderResource result{resource.name, stage, type};
	const auto& spirvType = compiler.get_type_from_variable(resource.id);
	result.array_size = spirvType.array.size() ? spirvType.array[0] : 1;
	result.vec_size = spirvType.vecsize;

	switch(type) {
		case ShaderResourceType::BufferUniform:
		case ShaderResourceType::BufferStorage:
			result.size = compiler.get_declared_struct_size_runtime_array(spirvType, 0); // TODO: specify runtime array size
			break;
		default: break;
	}

	result.location = compiler.get_decoration(resource.id, spv::DecorationLocation);
	result.set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
	result.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
	result.input_attachment_index = compiler.get_decoration(resource.id, spv::DecorationInputAttachmentIndex);
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

Util::Reference<Shader> createNormalToColorShader() {
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

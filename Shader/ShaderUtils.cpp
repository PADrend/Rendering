/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "ShaderUtils.h"
#include "Shader.h"
#include "../State/ShaderLayout.h"
#include "../Helper.h"
#include "../Core/Device.h"
#include "../Texture/Texture.h"
#include "../Buffer/BufferObject.h"
#include <Util/References.h>
#include <Util/StringUtils.h>
#include <Util/TypeConstant.h>
#include <Util/Macros.h>
#include <string>

#include <spirv_cross.hpp>
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

namespace Rendering {

//-------------

namespace ShaderUtils {

//-------------

static Util::TypeConstant getBaseType(const spirv_cross::SPIRType& type) {
	switch(type.basetype) {
		case spirv_cross::SPIRType::Boolean: return Util::TypeConstant::BOOL;
		case spirv_cross::SPIRType::SByte: return Util::TypeConstant::INT8;
		case spirv_cross::SPIRType::Char:
		case spirv_cross::SPIRType::UByte: return Util::TypeConstant::UINT8;
		case spirv_cross::SPIRType::Short: return Util::TypeConstant::INT16;
		case spirv_cross::SPIRType::UShort: return Util::TypeConstant::UINT16;
		case spirv_cross::SPIRType::Int: return Util::TypeConstant::INT32;
		case spirv_cross::SPIRType::UInt: return Util::TypeConstant::UINT32;
		case spirv_cross::SPIRType::Int64: return Util::TypeConstant::INT64;
		case spirv_cross::SPIRType::UInt64: return Util::TypeConstant::UINT64;
		case spirv_cross::SPIRType::Half: return Util::TypeConstant::HALF;
		case spirv_cross::SPIRType::Float: return Util::TypeConstant::FLOAT;
		case spirv_cross::SPIRType::Double: return Util::TypeConstant::DOUBLE;
		default: return Util::TypeConstant::UINT32;
	}
}

//-------------

static Util::ResourceFormat getResourceFormat(const std::string& baseName, spirv_cross::Compiler& compiler, const spirv_cross::SPIRType& type) {
	Util::ResourceFormat result;
	uint32_t member_count = type.member_types.size();

	for (uint32_t i = 0; i < member_count; ++i) {
		auto &member_type = compiler.get_type(type.member_types[i]);
		auto name = baseName.empty() ? compiler.get_member_name(type.self, i) : baseName + "." + compiler.get_member_name(type.self, i);
		auto offset = compiler.type_struct_member_offset(type, i);
		uint32_t count = 1;
		auto attrType = getBaseType(member_type);
		uint32_t array_stride = 0;

		if(!member_type.array.empty()) {
			array_stride = compiler.type_struct_member_array_stride(type, i);
			count = member_type.array[0];
		}

		if(member_type.vecsize > 0)
			count *= member_type.vecsize;
		if(member_type.columns > 0)
			count *= member_type.columns;

		if(member_type.basetype == spirv_cross::SPIRType::Struct) {
			auto structFormat = getResourceFormat("", compiler, member_type);

			if(!member_type.array.empty()) {
				// unroll arrays of struct
				for(uint32_t j=0; j<member_type.array[0]; ++j) {
					for(auto& attr : structFormat.getAttributes()) {
						result._appendAttribute(
							{name + "[" + Util::StringUtils::toString(j) + "]." + attr.getName()},
							attr.getDataType(),
							attr.getComponentCount(),
							attr.isNormalized(),
							attr.getInternalType(),
							offset + j*array_stride + attr.getOffset()
						);
					}
				}
			} else {
				// unroll struct
				for(auto& attr : structFormat.getAttributes()) {
					result._appendAttribute(
						{name + "." + attr.getName()},
						attr.getDataType(),
						attr.getComponentCount(),
						attr.isNormalized(),
						attr.getInternalType(),
						offset + attr.getOffset()
					);
				}
			}			
		} else {
			result._appendAttribute({name}, attrType, count, false, 0, offset);
		}		
	}
	return result;
}

//-------------

static ShaderResource readPushConstant(spirv_cross::Compiler& compiler, spirv_cross::Resource& resource, ShaderStage stage) {
	ShaderResource result{compiler.get_name(resource.base_type_id), 0, 0, {ShaderResourceType::PushConstant, stage}};
	const auto& spirvType = compiler.get_type_from_variable(resource.id);
	result.size = compiler.get_declared_struct_size_runtime_array(spirvType, 0); // TODO: specify runtime array size
	result.offset = std::numeric_limits<std::uint32_t>::max();
	for(auto i=0u; i < spirvType.member_types.size(); ++i) 
		result.offset = std::min(result.offset, compiler.get_member_decoration(spirvType.self, i, spv::DecorationOffset));
	result.size -= result.offset;

	result.format = getResourceFormat(resource.name, compiler, compiler.get_type(resource.base_type_id));

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

	switch(type) {
		case ShaderResourceType::BufferUniform:
		case ShaderResourceType::BufferStorage:
			result.size = compiler.get_declared_struct_size_runtime_array(spirvType, 0);
			result.format = getResourceFormat(compiler.get_name(resource.id), compiler, compiler.get_type(resource.base_type_id));
			break;
		default: break;
	}
		
	result.location = compiler.get_decoration(resource.id, spv::DecorationLocation);
	result.set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
	result.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
	result.inputAttachmentIndex = compiler.get_decoration(resource.id, spv::DecorationInputAttachmentIndex);
	
	result.vecSize = spirvType.vecsize;
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
	const auto& locator = getDataLocator();
	auto result = locator.locateFile(Util::FileName("./shader/DefaultShader.glsl"));
	WARN_AND_RETURN_IF(!result.first, "Could not find default shader.", nullptr);
	return Shader::loadShader(device, result.second, result.second);
}

//-------------

ShaderRef createPassThroughShader(const DeviceRef& device) {
	const auto& locator = getDataLocator();
	auto result = locator.locateFile(Util::FileName("./shader/PassThroughShader.glsl"));
	WARN_AND_RETURN_IF(!result.first, "Could not find pass thorugh shader.", nullptr);
	auto shader = Shader::createShader();
	shader->attachShaderObject(ShaderObjectInfo::loadVertex(result.second));
	return shader;
}

//-------------

ShaderRef createNormalToColorShader(const DeviceRef& device) {
	const auto& locator = getDataLocator();
	auto result = locator.locateFile(Util::FileName("./shader/NormalToColorShader.glsl"));
	WARN_AND_RETURN_IF(!result.first, "Could not find default shader.", nullptr);
	return Shader::loadShader(device, result.second, result.second);
}

//-------------

ShaderRef createNormalToColorShader() {
	return createNormalToColorShader(Device::getDefault());
}

//-------------

BindingState initBindingState(const ShaderLayout& layout, const BufferObjectRef& bo, const TextureRef& tex) {
	BindingState state;
	for(auto& set : layout.getLayoutSets()) {
		for(auto& layout : set.second.getLayouts()) {
			switch(layout.second.type) {
				case ShaderResourceType::BufferStorage:
				case ShaderResourceType::BufferUniform:
					for(uint32_t i=0; i<layout.second.elementCount; ++i)
						state.bind(bo, set.first, layout.first, i);
				case ShaderResourceType::Image:
				case ShaderResourceType::ImageSampler:
				case ShaderResourceType::ImageStorage:
				case ShaderResourceType::Sampler:
					for(uint32_t i=0; i<layout.second.elementCount; ++i)
						state.bind(tex, set.first, layout.first, i);
				default:
					break;
			}
		}
	}
	return state;
}

//-------------
}
}

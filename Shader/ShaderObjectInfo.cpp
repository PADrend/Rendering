/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	Copyright (C) 2019-2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "ShaderObjectInfo.h"
#include "../Helper.h"
#include "../Core/Device.h"
#include <Util/IO/FileUtils.h>
#include <Util/IO/FileLocator.h>
#include <Util/Macros.h>
#include <cstddef>
#include <vector>
#include <memory>

#include <shaderc/shaderc.hpp>
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#include <iostream>

namespace Rendering {
using namespace ShaderUtils;
	
const uint32_t ShaderObjectInfo::SHADER_STAGE_VERTEX = static_cast<uint32_t>(ShaderStage::Vertex);
const uint32_t ShaderObjectInfo::SHADER_STAGE_FRAGMENT = static_cast<uint32_t>(ShaderStage::Fragment);
const uint32_t ShaderObjectInfo::SHADER_STAGE_GEOMETRY = static_cast<uint32_t>(ShaderStage::Geometry);
const uint32_t ShaderObjectInfo::SHADER_STAGE_TESS_CONTROL = static_cast<uint32_t>(ShaderStage::TessellationControl);
const uint32_t ShaderObjectInfo::SHADER_STAGE_TESS_EVALUATION = static_cast<uint32_t>(ShaderStage::TessellationEvaluation);
const uint32_t ShaderObjectInfo::SHADER_STAGE_COMPUTE = static_cast<uint32_t>(ShaderStage::Compute);
	
//-------------

class ShaderIncluder : public shaderc::CompileOptions::IncluderInterface {
public:
	using Ptr = std::unique_ptr<shaderc::CompileOptions::IncluderInterface>;

	ShaderIncluder(const Util::FileName& file) {
		locator.addSearchPath(file.getPath());
	}

	// Handles shaderc_include_resolver_fn callbacks.
	virtual shaderc_include_result* GetInclude(const char* requested_source,
																							shaderc_include_type type,
																							const char* requesting_source,
																							size_t include_depth);

	// Handles shaderc_include_result_release_fn callbacks.
	virtual void ReleaseInclude(shaderc_include_result* data) { delete data; }

	Util::FileLocator locator;
};

//-------------

shaderc_include_result* ShaderIncluder::GetInclude(const char* requested_source, shaderc_include_type type, const char* requesting_source, size_t include_depth) {
	auto* result = new shaderc_include_result;
	result->content = nullptr;
	result->content_length = 0;
	result->source_name = nullptr;
	result->source_name_length = 0;
	result->user_data = nullptr;
	auto includeFile = locator.locateFile(Util::FileName(requested_source));
	WARN("ShaderObjectInfo: #include not supported. Include file: '" + std::string(requested_source) + "' " + (includeFile.first ? "found" : "not found"));
	return result; 
}

//-------------

ShaderObjectInfo::ShaderObjectInfo(ShaderStage stage, std::string source) : stage(stage), source(std::move(source)) { }

//-------------

ShaderObjectInfo::ShaderObjectInfo(ShaderStage stage, std::vector<uint32_t> code) : stage(stage), code(std::move(code)) { }

//-------------

bool ShaderObjectInfo::compile(const DeviceRef& device) {
	if(source.empty()) {
		WARN("ShaderObjectInfo: There is no source code to be compiled.");
		return false;
	}
	code.clear();

	vk::Device vkDevice(device->getApiHandle());
	
	shaderc::Compiler compiler;
	shaderc::CompileOptions options;
	options.SetGenerateDebugInfo();
	options.SetOptimizationLevel(shaderc_optimization_level_performance);
	options.SetAutoMapLocations(true);
	options.SetAutoBindUniforms(true);
	
	ShaderIncluder::Ptr includer(new ShaderIncluder(filename));
	options.SetIncluder(std::move(includer));

	shaderc_shader_kind kind;
	switch(stage) {
		case ShaderStage::Vertex:
			options.AddMacroDefinition("SG_VERTEX_SHADER");
			kind = shaderc_glsl_vertex_shader;
			break;
		case ShaderStage::TessellationControl:
			options.AddMacroDefinition("SG_TESSELLATION_CONTROL_SHADER");
			kind = shaderc_glsl_tess_control_shader;
			break;
		case ShaderStage::TessellationEvaluation:
			options.AddMacroDefinition("SG_TESSELLATION_EVALUATION_SHADER");
			kind = shaderc_glsl_tess_evaluation_shader;
			break;
		case ShaderStage::Geometry:
			options.AddMacroDefinition("SG_GEOMETRY_SHADER");
			kind = shaderc_glsl_geometry_shader;
			break;
		case ShaderStage::Fragment:
			options.AddMacroDefinition("SG_FRAGMENT_SHADER");
			kind = shaderc_glsl_fragment_shader;
			break;
		case ShaderStage::Compute:
			options.AddMacroDefinition("SG_COMPUTE_SHADER");
			kind = shaderc_glsl_compute_shader;
			break;
		default:
			WARN("ShaderObjectInfo: Invalid shader stage '" + toString(stage) + "'");
			return false;
	}
	for(auto& define : defines)
		options.AddMacroDefinition(define.first, define.second);

	std::string name = toString(stage);
	if(!filename.empty())
		name = filename.toString();
	
	shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(source, kind, name.c_str(), options);
	if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
		if(filename.empty())
			WARN(std::string("Shader compile error:\n") + result.GetErrorMessage() + "\nShader code:\n" + source);
		else
			WARN(std::string("Shader compile error:\n") + result.GetErrorMessage() + "\nin shader file: " + filename.toShortString() + "\n");
		return false;
	}
	code.insert(code.end(), result.cbegin(), result.cend());

	return true;
}

//-------------

ShaderObjectInfo ShaderObjectInfo::createVertex(const std::vector<uint32_t>& spirv) {
	return ShaderObjectInfo(ShaderStage::Vertex, spirv);
}

//-------------

ShaderObjectInfo ShaderObjectInfo::createFragment(const std::vector<uint32_t>& spirv) {
	return ShaderObjectInfo(ShaderStage::Fragment, spirv);
}

//-------------

ShaderObjectInfo ShaderObjectInfo::createGeometry(const std::vector<uint32_t>& spirv) {
	return ShaderObjectInfo(ShaderStage::Geometry, spirv);
}

//-------------

ShaderObjectInfo ShaderObjectInfo::createCompute(const std::vector<uint32_t>& spirv) {
	return ShaderObjectInfo(ShaderStage::Compute, spirv);
}

//-------------

ShaderObjectInfo ShaderObjectInfo::createVertex(const std::string & code) {
	return ShaderObjectInfo(ShaderStage::Vertex, code);
}

//-------------

ShaderObjectInfo ShaderObjectInfo::createFragment(const std::string & code) {
	return ShaderObjectInfo(ShaderStage::Fragment, code);
}

//-------------

ShaderObjectInfo ShaderObjectInfo::createGeometry(const std::string & code) {
	return ShaderObjectInfo(ShaderStage::Geometry, code);
}

//-------------

ShaderObjectInfo ShaderObjectInfo::createCompute(const std::string & code) {
	return ShaderObjectInfo(ShaderStage::Compute, code);
}

//-------------

ShaderObjectInfo ShaderObjectInfo::loadVertex(const Util::FileName & file) {
	return createVertex(Util::FileUtils::getParsedFileContents(file)).setFileName(file);
}

//-------------

ShaderObjectInfo ShaderObjectInfo::loadFragment(const Util::FileName & file) {
	return createFragment(Util::FileUtils::getParsedFileContents(file)).setFileName(file);
}

//-------------

ShaderObjectInfo ShaderObjectInfo::loadGeometry(const Util::FileName & file) {
	return createGeometry(Util::FileUtils::getParsedFileContents(file)).setFileName(file);
}

//-------------

ShaderObjectInfo ShaderObjectInfo::loadCompute(const Util::FileName & file) {
	return createCompute(Util::FileUtils::getParsedFileContents(file)).setFileName(file);
}

//-------------

}

//-------------

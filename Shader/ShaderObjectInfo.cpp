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
#include <Util/StringUtils.h>
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

static std::vector<std::string> split(const std::string & subject, const std::string & delimiter, int max=-1){
	std::vector<std::string> result;
	const size_t len = subject.length();
	if(len>0){
		const size_t delimiterLen = delimiter.length();
		if(delimiterLen>len || delimiterLen==0){
			result.emplace_back(subject);
		}else{
			size_t cursor = 0;
			for( int i = 1 ; i!=max&&cursor<=len-delimiterLen ; ++i){
				size_t pos = subject.find(delimiter,cursor);
				if( pos==std::string::npos ) // no delimiter found? -> to the end
					pos = len;
				result.push_back( subject.substr(cursor,pos-cursor) );
				cursor = pos+delimiterLen;

				if(cursor==len){ // ending on delimiter? -> add empty part
					result.push_back("");
				}
			}
			if(cursor<len)
				result.push_back( subject.substr(cursor,len-cursor) );
		}
	}
	return result;
}

//-------------

static std::string printErrorLines(const std::string& error, const std::string& source, const std::string& name) {
	std::stringstream ss;
	std::vector<std::string> errLines = split(error, "\n");
	std::vector<std::string> srcLines = split(source, "\n");
	std::set<uint32_t> lines;
	// collect lines
	for(auto& err : errLines) {
		// usual error format: <name>:<line>: error: ...
		auto prefix = split(err, ":");
		uint32_t line;
		if(prefix.size() >= 2 && (line = Util::StringUtils::toNumber<uint32_t>(prefix[1])) > 0 && Util::StringUtils::beginsWith(prefix[0].c_str(), name.c_str())) {
			if(line > 1 && line <= srcLines.size())
				lines.emplace(line-1);
			if(line <= srcLines.size())
				lines.emplace(line);
			if(line+1 <= srcLines.size())
				lines.emplace(line+1);
		}
	}

	for(auto line : lines) {
		if(lines.count(line-1) == 0)
			ss << "  ..." << std::endl;
		ss << "  " << line << ": " << srcLines[line-1] << std::endl;
	}
	if(!lines.empty())
		ss << "  ..." << std::endl;
	return ss.str();
};

//-------------

class ShaderIncluder : public shaderc::CompileOptions::IncluderInterface {
public:
	using Ptr = std::unique_ptr<shaderc::CompileOptions::IncluderInterface>;
	struct IncludeResult {
		explicit IncludeResult(const std::string& source, const std::string& name) : source(source), name(name) {}
		std::string source;
		std::string name;
	};

	ShaderIncluder(const Util::FileName& file) : locator(getDataLocator()) {
		locator.addSearchPath(file.getDir());
	}

	// Handles shaderc_include_resolver_fn callbacks.
	virtual shaderc_include_result* GetInclude(const char* requested_source,
																							shaderc_include_type type,
																							const char* requesting_source,
																							size_t include_depth);

	// Handles shaderc_include_result_release_fn callbacks.
	virtual void ReleaseInclude(shaderc_include_result* data) {
		if(data) {
			delete reinterpret_cast<IncludeResult*>(data->user_data);
			delete data;
		}
	}

	Util::FileLocator locator;
};

//-------------

shaderc_include_result* ShaderIncluder::GetInclude(const char* requested_source, shaderc_include_type type, const char* requesting_source, size_t include_depth) {
	auto includeFile = locator.locateFile(Util::FileName(requested_source));
	IncludeResult* userData = nullptr;
	if(includeFile.first)
		userData = new IncludeResult(Util::FileUtils::getFileContents(includeFile.second), requested_source);
	
	auto* result = new shaderc_include_result;
	result->user_data = userData;
	result->content = userData ? userData->source.c_str() : nullptr;
	result->content_length = userData ? userData->source.length() : 0;
	result->source_name = userData ? userData->name.c_str() : nullptr;
	result->source_name_length = userData ? userData->name.length() : 0;
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
	
	shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(source, kind, name.c_str(), options);
	if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
		std::stringstream ss;
		ss << "Shader compile error";
		if(!filename.empty())
			ss << " in file '" << filename.toString() << "'";
		ss << ":" << std::endl;
		ss << result.GetErrorMessage();
		ss << printErrorLines(result.GetErrorMessage(), source, name) << std::endl;
		WARN(ss.str());
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

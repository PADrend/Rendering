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
#include <regex>
#include <sstream>
#include <iostream>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#include <SPIRV/GlslangToSpv.h>

namespace Rendering {
using namespace ShaderUtils;
	
const uint32_t ShaderObjectInfo::SHADER_STAGE_VERTEX = static_cast<uint32_t>(ShaderStage::Vertex);
const uint32_t ShaderObjectInfo::SHADER_STAGE_FRAGMENT = static_cast<uint32_t>(ShaderStage::Fragment);
const uint32_t ShaderObjectInfo::SHADER_STAGE_GEOMETRY = static_cast<uint32_t>(ShaderStage::Geometry);
const uint32_t ShaderObjectInfo::SHADER_STAGE_TESS_CONTROL = static_cast<uint32_t>(ShaderStage::TessellationControl);
const uint32_t ShaderObjectInfo::SHADER_STAGE_TESS_EVALUATION = static_cast<uint32_t>(ShaderStage::TessellationEvaluation);
const uint32_t ShaderObjectInfo::SHADER_STAGE_COMPUTE = static_cast<uint32_t>(ShaderStage::Compute);
const uint32_t ShaderObjectInfo::SHADER_STAGE_TASK = static_cast<uint32_t>(ShaderStage::Undefined);
const uint32_t ShaderObjectInfo::SHADER_STAGE_MESH = static_cast<uint32_t>(ShaderStage::Undefined);

//-------------

static void initResources(TBuiltInResource &Resources) {
	Resources.maxLights = 32;
	Resources.maxClipPlanes = 6;
	Resources.maxTextureUnits = 32;
	Resources.maxTextureCoords = 32;
	Resources.maxVertexAttribs = 64;
	Resources.maxVertexUniformComponents = 4096;
	Resources.maxVaryingFloats = 64;
	Resources.maxVertexTextureImageUnits = 32;
	Resources.maxCombinedTextureImageUnits = 80;
	Resources.maxTextureImageUnits = 32;
	Resources.maxFragmentUniformComponents = 4096;
	Resources.maxDrawBuffers = 32;
	Resources.maxVertexUniformVectors = 128;
	Resources.maxVaryingVectors = 8;
	Resources.maxFragmentUniformVectors = 16;
	Resources.maxVertexOutputVectors = 16;
	Resources.maxFragmentInputVectors = 15;
	Resources.minProgramTexelOffset = -8;
	Resources.maxProgramTexelOffset = 7;
	Resources.maxClipDistances = 8;
	Resources.maxComputeWorkGroupCountX = 65535;
	Resources.maxComputeWorkGroupCountY = 65535;
	Resources.maxComputeWorkGroupCountZ = 65535;
	Resources.maxComputeWorkGroupSizeX = 1024;
	Resources.maxComputeWorkGroupSizeY = 1024;
	Resources.maxComputeWorkGroupSizeZ = 64;
	Resources.maxComputeUniformComponents = 1024;
	Resources.maxComputeTextureImageUnits = 16;
	Resources.maxComputeImageUniforms = 8;
	Resources.maxComputeAtomicCounters = 8;
	Resources.maxComputeAtomicCounterBuffers = 1;
	Resources.maxVaryingComponents = 60;
	Resources.maxVertexOutputComponents = 64;
	Resources.maxGeometryInputComponents = 64;
	Resources.maxGeometryOutputComponents = 128;
	Resources.maxFragmentInputComponents = 128;
	Resources.maxImageUnits = 8;
	Resources.maxCombinedImageUnitsAndFragmentOutputs = 8;
	Resources.maxCombinedShaderOutputResources = 8;
	Resources.maxImageSamples = 0;
	Resources.maxVertexImageUniforms = 0;
	Resources.maxTessControlImageUniforms = 0;
	Resources.maxTessEvaluationImageUniforms = 0;
	Resources.maxGeometryImageUniforms = 0;
	Resources.maxFragmentImageUniforms = 8;
	Resources.maxCombinedImageUniforms = 8;
	Resources.maxGeometryTextureImageUnits = 16;
	Resources.maxGeometryOutputVertices = 256;
	Resources.maxGeometryTotalOutputComponents = 1024;
	Resources.maxGeometryUniformComponents = 1024;
	Resources.maxGeometryVaryingComponents = 64;
	Resources.maxTessControlInputComponents = 128;
	Resources.maxTessControlOutputComponents = 128;
	Resources.maxTessControlTextureImageUnits = 16;
	Resources.maxTessControlUniformComponents = 1024;
	Resources.maxTessControlTotalOutputComponents = 4096;
	Resources.maxTessEvaluationInputComponents = 128;
	Resources.maxTessEvaluationOutputComponents = 128;
	Resources.maxTessEvaluationTextureImageUnits = 16;
	Resources.maxTessEvaluationUniformComponents = 1024;
	Resources.maxTessPatchComponents = 120;
	Resources.maxPatchVertices = 32;
	Resources.maxTessGenLevel = 64;
	Resources.maxViewports = 16;
	Resources.maxVertexAtomicCounters = 0;
	Resources.maxTessControlAtomicCounters = 0;
	Resources.maxTessEvaluationAtomicCounters = 0;
	Resources.maxGeometryAtomicCounters = 0;
	Resources.maxFragmentAtomicCounters = 8;
	Resources.maxCombinedAtomicCounters = 8;
	Resources.maxAtomicCounterBindings = 1;
	Resources.maxVertexAtomicCounterBuffers = 0;
	Resources.maxTessControlAtomicCounterBuffers = 0;
	Resources.maxTessEvaluationAtomicCounterBuffers = 0;
	Resources.maxGeometryAtomicCounterBuffers = 0;
	Resources.maxFragmentAtomicCounterBuffers = 1;
	Resources.maxCombinedAtomicCounterBuffers = 1;
	Resources.maxAtomicCounterBufferSize = 16384;
	Resources.maxTransformFeedbackBuffers = 4;
	Resources.maxTransformFeedbackInterleavedComponents = 64;
	Resources.maxCullDistances = 8;
	Resources.maxCombinedClipAndCullDistances = 8;
	Resources.maxSamples = 4;
	Resources.maxMeshOutputVerticesNV = 256;
	Resources.maxMeshOutputPrimitivesNV = 512;
	Resources.maxMeshWorkGroupSizeX_NV = 32;
	Resources.maxMeshWorkGroupSizeY_NV = 1;
	Resources.maxMeshWorkGroupSizeZ_NV = 1;
	Resources.maxTaskWorkGroupSizeX_NV = 32;
	Resources.maxTaskWorkGroupSizeY_NV = 1;
	Resources.maxTaskWorkGroupSizeZ_NV = 1;
	Resources.maxMeshViewCountNV = 4;
	Resources.limits.nonInductiveForLoops = 1;
	Resources.limits.whileLoops = 1;
	Resources.limits.doWhileLoops = 1;
	Resources.limits.generalUniformIndexing = 1;
	Resources.limits.generalAttributeMatrixVectorIndexing = 1;
	Resources.limits.generalVaryingIndexing = 1;
	Resources.limits.generalSamplerIndexing = 1;
	Resources.limits.generalVariableIndexing = 1;
	Resources.limits.generalConstantMatrixVectorIndexing = 1;
}

//-------------

static EShLanguage getShaderLang(const ShaderStage& stage) {
	switch(stage) {
		case ShaderStage::Vertex: return EShLanguage::EShLangVertex;
		case ShaderStage::TessellationControl: return EShLanguage::EShLangTessControl;
		case ShaderStage::TessellationEvaluation: return EShLanguage::EShLangTessEvaluation;
		case ShaderStage::Geometry: return EShLanguage::EShLangGeometry;
		case ShaderStage::Fragment: return EShLanguage::EShLangFragment;
		case ShaderStage::Compute: return EShLanguage::EShLangCompute;
		default:
			WARN("ShaderObjectInfo: Invalid shader stage '" + toString(stage) + "'");
			return EShLanguage::EShLangCount;
	}
}

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

class ShaderIncluder : public glslang::TShader::Includer {
public:
	using Ptr = std::unique_ptr<glslang::TShader::Includer>;

	ShaderIncluder(const Util::FileName& file) : locator(getDataLocator()) {
		locator.addSearchPath(file.getDir());
	}

	// For the "system" or <>-style includes; search the "system" paths.
	IncludeResult* includeSystem(const char* headerName, const char* includerName, size_t inclusionDepth) override {
		return nullptr;
	}

	// For the "local"-only aspect of a "" include.
	IncludeResult* includeLocal(const char* headerName, const char* includerName, size_t inclusionDepth) override;

	// Signals that the parser will no longer use the contents of the specified IncludeResult.
	void releaseInclude(IncludeResult* result) override {
		if(result) {
			delete [] static_cast<ShaderSourceData*>(result->userData);
			delete result;
		}
	};

	using ShaderSourceData = char;
	Util::FileLocator locator;
};

//-------------

ShaderIncluder::IncludeResult* ShaderIncluder::includeLocal(const char* headerName, const char* includerName, size_t inclusionDepth) {
	auto includeFile = locator.locateFile(Util::FileName(headerName));
	if(includeFile.first) {
		auto file = Util::FileUtils::openForReading(includeFile.second);
		if(file) {
			file->seekg(0, std::ios::end);
			size_t length = file->tellg();
			file->seekg(0, std::ios::beg);
			ShaderSourceData* content = new ShaderSourceData[length];
			file->read(content, length);
			return new IncludeResult(includeFile.second.getPath(), content, length, content);
		}
	}
	return nullptr;
}

//-------------

ShaderObjectInfo::ShaderObjectInfo(ShaderStage stage, std::string source) : stage(stage), source(std::move(source)) { }

//-------------

ShaderObjectInfo::ShaderObjectInfo(ShaderStage stage, std::vector<uint32_t> code) : stage(stage), code(std::move(code)) { }

//-------------

bool ShaderObjectInfo::compile(const DeviceRef& device) {
	using namespace glslang;

	if(source.empty()) {
		WARN("ShaderObjectInfo: There is no source code to be compiled.");
		return false;
	}
	code.clear();

	vk::Device vkDevice(device->getApiHandle());

	EShLanguage kind = getShaderLang(stage);
	std::string name = toString(stage);
	WARN_AND_RETURN_IF(kind == EShLangCount, "ShaderObjectInfo: Invalid shader stage '" + name + "'", false);

	EShMessages messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);
	TBuiltInResource resources{};
	initResources(resources);
	ShaderIncluder includer(filename);
	
	std::stringstream ssHeader;
	switch(stage) {
		case ShaderStage::Vertex:
			ssHeader << "#define SG_VERTEX_SHADER" << std::endl;
			break;
		case ShaderStage::TessellationControl:
			ssHeader << "#define SG_TESSELLATION_CONTROL_SHADER" << std::endl;
			break;
		case ShaderStage::TessellationEvaluation:
			ssHeader << "#define SG_TESSELLATION_EVALUATION_SHADER" << std::endl;
			break;
		case ShaderStage::Geometry:
			ssHeader << "#define SG_GEOMETRY_SHADER" << std::endl;
			break;
		case ShaderStage::Fragment:
			ssHeader << "#define SG_FRAGMENT_SHADER" << std::endl;
			break;
		case ShaderStage::Compute:
			ssHeader << "#define SG_COMPUTE_SHADER" << std::endl;
			break;
	}
	for(auto& define : defines) {
			ssHeader << "#define " << define.first << " " << define.second<< std::endl;
	}

	std::string header = ssHeader.str();
	const char* sources[1] = { source.c_str() };
	const char* files[1] = { filename.getFile().c_str() };

	TShader shader(kind);
	shader.setEnvInput(EShSourceGlsl, kind, EShClientVulkan, 100);
	shader.setEnvClient(EShClientVulkan, EShTargetVulkan_1_2);
	shader.setEnvTarget(EshTargetSpv, EShTargetSpv_1_6);
	shader.setPreamble(header.c_str());
	shader.setStringsWithLengthsAndNames(sources, nullptr, files, 1);
	if(!shader.parse(&resources, 110, true, messages)) {
		std::stringstream ss;
		ss << "Shader compile error";
		if(!filename.empty())
			ss << " in file '" << filename.toString() << "'";
		ss << ":" << std::endl;
		ss << shader.getInfoLog() << std::endl;
		ss << shader.getInfoDebugLog();
		//ss << printErrorLines(result.GetErrorMessage(), source, name) << std::endl;
		WARN_AND_RETURN(ss.str(), false);
	}

	TProgram program;
	program.addShader(&shader);
	if(!program.link(messages)){
		std::stringstream ss;
		ss << "Program linker error";
		if(!filename.empty())
			ss << " in file '" << filename.toString() << "'";
		ss << ":" << std::endl;
		ss << program.getInfoLog() << std::endl;
		ss << program.getInfoDebugLog();
		WARN_AND_RETURN(ss.str(), false);
	}

	GlslangToSpv(*program.getIntermediate(kind), code);

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

ShaderObjectInfo ShaderObjectInfo::createMesh(const std::string & code) {
	throw std::logic_error("No support for GL_NV_mesh_shader.");
}

ShaderObjectInfo ShaderObjectInfo::createTask(const std::string & code) {
	throw std::logic_error("No support for GL_NV_mesh_shader.");
}

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

ShaderObjectInfo ShaderObjectInfo::loadMesh(const Util::FileName & file) {
	return createMesh(Util::FileUtils::getParsedFileContents(file)).setFileName(file);
}

ShaderObjectInfo ShaderObjectInfo::loadTask(const Util::FileName & file) {
	return createTask(Util::FileUtils::getParsedFileContents(file)).setFileName(file);
}

}

//-------------

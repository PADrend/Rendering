/*
	This file is part of the Platform for Algorithm Development and Rendering (PADrend).
	Web page: http://www.padrend.de/
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	Copyright (C) 2014-2022 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "ShaderCompiler.h"
#include "../RenderDevice.h"
#include "../Helper.h"

#include <Util/Macros.h>
#include <Util/IO/FileUtils.h>
#include <Util/StringUtils.h>

#include <SPIRV/GlslangToSpv.h>

#include <nvrhi/nvrhi.h>

namespace Rendering {
namespace {

// TODO: init limits from device
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

EShLanguage getShaderLang(const nvrhi::ShaderType& stage) {
	switch(stage) {       
		case nvrhi::ShaderType::Compute: return EShLanguage::EShLangCompute;
		case nvrhi::ShaderType::Vertex: return EShLanguage::EShLangVertex;
		case nvrhi::ShaderType::Hull: return EShLanguage::EShLangTessControl;
		case nvrhi::ShaderType::Domain: return EShLanguage::EShLangTessEvaluation;
		case nvrhi::ShaderType::Geometry: return EShLanguage::EShLangGeometry;
		case nvrhi::ShaderType::Pixel: return EShLanguage::EShLangFragment;
		case nvrhi::ShaderType::Amplification: return EShLanguage::EShLangTaskNV;
		case nvrhi::ShaderType::Mesh: return EShLanguage::EShLangMeshNV;
		case nvrhi::ShaderType::RayGeneration: return EShLanguage::EShLangRayGen;
		case nvrhi::ShaderType::AnyHit: return EShLanguage::EShLangAnyHit;
		case nvrhi::ShaderType::ClosestHit: return EShLanguage::EShLangClosestHit;
		case nvrhi::ShaderType::Miss: return EShLanguage::EShLangMiss;
		case nvrhi::ShaderType::Intersection: return EShLanguage::EShLangIntersect;
		case nvrhi::ShaderType::Callable: return EShLanguage::EShLangCallable;
		default:
			WARN("ShaderObjectInfo: Invalid shader stage '" + toString(stage) + "'");
			return EShLanguage::EShLangCount;
	}
}

//-------------

class ShaderIncluder : public glslang::TShader::Includer {
public:
	using Ptr = std::unique_ptr<glslang::TShader::Includer>;

	ShaderIncluder(const Util::FileName& file, const Util::FileLocator& loc) : locator(loc) {
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

}

//-------------

ShaderCompilerGLSL::ShaderCompilerGLSL() : locator(getDataLocator()) {
}

//-------------

bool ShaderCompilerGLSL::compile(const Util::FileName& file, nvrhi::ShaderType type, std::vector<uint32_t>& outByteCode) {
	return false;
}

//-------------

bool ShaderCompilerGLSL::compile(const std::string& source, nvrhi::ShaderType type, std::vector<uint32_t>& outByteCode) {
	return false;
}

//-------------

bool ShaderCompilerGLSL::compileLibrary(const Util::FileName& file, std::vector<uint32_t>& outByteCode) {
	return false;
}

//-------------

bool ShaderCompilerGLSL::compileLibrary(const std::string& source, std::vector<uint32_t>& outByteCode) {
	return false;
}

//-------------

} // Rendering
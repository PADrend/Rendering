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
#ifndef RENDERING_SHADER_SHADEROBJECTINFO_H
#define RENDERING_SHADER_SHADEROBJECTINFO_H

#include "../Core/ApiHandles.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <Util/IO/FileName.h>

// Forward declarations
namespace Util {
class FileName;
}
namespace Rendering {
class Device;
using DeviceRef = Util::Reference<Device>;

enum class ShaderStage : uint8_t {
	Undefined = 0,
	Vertex = 1 << 0,
	TesselationControl = 1 << 1,
	TesselationEvaluation = 1 << 2,
	Geometry = 1 << 3,
	Fragment = 1 << 4,
	Compute = 1 << 5,
};

inline ShaderStage operator|(ShaderStage a, ShaderStage b) {
	return static_cast<ShaderStage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline ShaderStage operator&(ShaderStage a, ShaderStage b) {
	return static_cast<ShaderStage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

enum class ShaderResourceType {
	Input,
	InputAttachment,
	Output,
	Image,
	ImageSampler,
	ImageStorage,
	Sampler,
	BufferUniform,
	BufferStorage,
	PushConstant,
	SpecializationConstant
};

struct ShaderResource {
	std::string name;
	ShaderStage stages;
	ShaderResourceType type;
	uint32_t set;
	uint32_t binding;
	uint32_t location;
	uint32_t input_attachment_index;
	uint32_t vec_size;
	uint32_t columns;
	uint32_t array_size;
	uint32_t offset;
	uint32_t size;
	uint32_t constant_id;
	bool dynamic;
	std::string toString() const;
};

/**
 * Storage for shader type and shader code. There is no handle stored in
 * here. When compiling an object, the handle is returned and has to be stored
 * by the caller.
 * @ingroup shader
 */
class ShaderObjectInfo {
	public:
		static const uint32_t SHADER_STAGE_VERTEX;
		static const uint32_t SHADER_STAGE_FRAGMENT;
		static const uint32_t SHADER_STAGE_GEOMETRY;
		static const uint32_t SHADER_STAGE_TESS_CONTROL;
		static const uint32_t SHADER_STAGE_TESS_EVALUATION;
		static const uint32_t SHADER_STAGE_COMPUTE;
	private:
		struct Define {
			std::string key;
			std::string value;
		};
		const ShaderStage stage;
		std::string code;
		std::vector<Define> defines;
		std::vector<uint32_t> spirv;
		Util::FileName filename;

		ShaderObjectInfo(ShaderStage stage, std::string _code);
		ShaderObjectInfo(ShaderStage stage, std::vector<uint32_t> _spirv);

	public:
		const std::string & getCode() const {
			return code;
		}
		ShaderStage getType() const {
			return stage;
		}
		ShaderObjectInfo& addDefine(const std::string& key, const std::string& value="") {
			defines.emplace_back(Define{key, value});
			return *this;
		}

		/**
		 * Compile the source stored in this shader object.
		 * 
		 * @return Handle of the compiled shader.
		 */
		ShaderModuleHandle compile(const DeviceRef& device);
		ShaderModuleHandle compile();

		/**
		 * Compile the source stored in this shader object.
		 * 
		 * @return Handle of the compiled shader.
		 */
		std::vector<ShaderResource> reflect();

		/**
		 * Create a VertexShaderObject from the given spir-v code
		 * 
		 * @note Inserts "#define SG_VERTEX_SHADER" before the first line.
		 */
		static ShaderObjectInfo createVertex(const std::vector<uint32_t>& spirv);

		/**
		 * Create a FragmentShaderObject from the given spir-v code
		 * 
		 * @note Inserts "#define SG_FRAGMENT_SHADER" before the first line.
		 */
		static ShaderObjectInfo createFragment(const std::vector<uint32_t>& spirv);

		/**
		 * Create a GeometryShaderObject from the given spir-v code
		 * 
		 * @note Inserts "#define SG_GEOMETRY_SHADER" before the first line.
		 */
		static ShaderObjectInfo createGeometry(const std::vector<uint32_t>& spirv);

		/**
		 * Create a ComputeShaderObject from the given spir-v code
		 * 
		 * @note Inserts "#define SG_COMPUTE_SHADER" before the first line.
		 */
		static ShaderObjectInfo createCompute(const std::vector<uint32_t>& spirv);

		/**
		 * Create a VertexShaderObject from the given glsl code
		 * 
		 * @note Inserts "#define SG_VERTEX_SHADER" before the first line.
		 */
		static ShaderObjectInfo createVertex(const std::string & code);

		/**
		 * Create a FragmentShaderObject from the given glsl code
		 * 
		 * @note Inserts "#define SG_FRAGMENT_SHADER" before the first line.
		 */
		static ShaderObjectInfo createFragment(const std::string & code);

		/**
		 * Create a GeometryShaderObject from the given glsl code
		 * 
		 * @note Inserts "#define SG_GEOMETRY_SHADER" before the first line.
		 */
		static ShaderObjectInfo createGeometry(const std::string & code);

		/**
		 * Create a ComputeShaderObject from the given glsl code
		 * 
		 * @note Inserts "#define SG_COMPUTE_SHADER" before the first line.
		 */
		static ShaderObjectInfo createCompute(const std::string & code);

		//! Load a VertexShaderObject from the given file.
		static ShaderObjectInfo loadVertex(const Util::FileName & file);

		//! Load a FragmentShaderObject from the given file.
		static ShaderObjectInfo loadFragment(const Util::FileName & file);

		//! Load a GeometryShaderObject from the given file.
		static ShaderObjectInfo loadGeometry(const Util::FileName & file);

		//! Load a ComputeShaderObject from the given file.
		static ShaderObjectInfo loadCompute(const Util::FileName & file);
	
		const Util::FileName & getFileName() const { return filename; }
	private:
		ShaderObjectInfo& setFileName(const Util::FileName & f) { filename=f; return *this; }
};

}

#endif // RENDERING_SHADER_SHADEROBJECTINFO_H



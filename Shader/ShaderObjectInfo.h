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
#include "ShaderUtils.h"

#include <cstdint>
#include <vector>

#include <Util/IO/FileName.h>

namespace Rendering {
class Device;
using DeviceRef = Util::Reference<Device>;

/**
 * Storage for shader type and shader code. There is no handle stored in
 * here. When compiling an object, the handle is returned and has to be stored
 * by the caller.
 * @ingroup shader
 */
class ShaderObjectInfo {
	public:
		RENDERINGAPI static const uint32_t SHADER_STAGE_VERTEX;
		RENDERINGAPI static const uint32_t SHADER_STAGE_FRAGMENT;
		RENDERINGAPI static const uint32_t SHADER_STAGE_GEOMETRY;
		RENDERINGAPI static const uint32_t SHADER_STAGE_TESS_CONTROL;
		RENDERINGAPI static const uint32_t SHADER_STAGE_TESS_EVALUATION;
		RENDERINGAPI static const uint32_t SHADER_STAGE_COMPUTE;
		RENDERINGAPI static const uint32_t SHADER_STAGE_TASK;
		RENDERINGAPI static const uint32_t SHADER_STAGE_MESH;
	private:
		using Define = std::pair<std::string, std::string>;
		const ShaderStage stage;
		std::string source;
		std::vector<Define> defines;
		std::vector<uint32_t> code;
		Util::FileName filename;

		ShaderObjectInfo(ShaderStage stage, std::string source);
		ShaderObjectInfo(ShaderStage stage, std::vector<uint32_t> code);

	public:
		const std::string& getSource() const {
			return source;
		}
		const std::vector<uint32_t>& getCode() const {
			return code;
		}
		ShaderStage getType() const {
			return stage;
		}
		ShaderObjectInfo& addDefine(const std::string& key, const std::string& value="") {
			defines.emplace_back(key, value);
			return *this;
		}

		bool isCompiled() const { return !code.empty(); }

		/**
		 * Compile the source stored in this shader object.
		 * 
		 * @return true on success, false otherwise
		 */
		RENDERINGAPI bool compile(const DeviceRef& device);

		/**
		 * Create a VertexShaderObject from the given spir-v code
		 * 
		 * @note Inserts "#define SG_VERTEX_SHADER" before the first line.
		 */
		RENDERINGAPI static ShaderObjectInfo createVertex(const std::vector<uint32_t>& spirv);

		/**
		 * Create a FragmentShaderObject from the given spir-v code
		 * 
		 * @note Inserts "#define SG_FRAGMENT_SHADER" before the first line.
		 */
		RENDERINGAPI static ShaderObjectInfo createFragment(const std::vector<uint32_t>& spirv);

		/**
		 * Create a GeometryShaderObject from the given spir-v code
		 * 
		 * @note Inserts "#define SG_GEOMETRY_SHADER" before the first line.
		 */
		RENDERINGAPI static ShaderObjectInfo createGeometry(const std::vector<uint32_t>& spirv);

		/**
		 * Create a ComputeShaderObject from the given spir-v code
		 * 
		 * @note Inserts "#define SG_COMPUTE_SHADER" before the first line.
		 */
		RENDERINGAPI static ShaderObjectInfo createCompute(const std::vector<uint32_t>& spirv);

		/**
		 * Create a VertexShaderObject from the given glsl code
		 * 
		 * @note Inserts "#define SG_VERTEX_SHADER" before the first line.
		 */
		RENDERINGAPI static ShaderObjectInfo createVertex(const std::string & code);

		/**
		 * Create a FragmentShaderObject from the given glsl code
		 * 
		 * @note Inserts "#define SG_FRAGMENT_SHADER" before the first line.
		 */
		RENDERINGAPI static ShaderObjectInfo createFragment(const std::string & code);

		/**
		 * Create a GeometryShaderObject from the given glsl code
		 * 
		 * @note Inserts "#define SG_GEOMETRY_SHADER" before the first line.
		 */
		RENDERINGAPI static ShaderObjectInfo createGeometry(const std::string & code);

		/**
		 * Create a ComputeShaderObject from the given glsl code
		 * 
		 * @note Inserts "#define SG_COMPUTE_SHADER" before the first line.
		 */
		RENDERINGAPI static ShaderObjectInfo createCompute(const std::string & code);

		/**
		 * Create a MeshShaderObject from the given code
		 * 
		 * @note Inserts "#define SG_MESH_SHADER" before the first line.
		 */
		RENDERINGAPI static ShaderObjectInfo createMesh(const std::string & code);

		/**
		 * Create a TaskShaderObject from the given code
		 * 
		 * @note Inserts "#define SG_TASK_SHADER" before the first line.
		 */
		RENDERINGAPI static ShaderObjectInfo createTask(const std::string & code);

		//! Load a VertexShaderObject from the given file.
		RENDERINGAPI static ShaderObjectInfo loadVertex(const Util::FileName & file);

		//! Load a FragmentShaderObject from the given file.
		RENDERINGAPI static ShaderObjectInfo loadFragment(const Util::FileName & file);

		//! Load a GeometryShaderObject from the given file.
		RENDERINGAPI static ShaderObjectInfo loadGeometry(const Util::FileName & file);

		//! Load a ComputeShaderObject from the given file.
		RENDERINGAPI static ShaderObjectInfo loadCompute(const Util::FileName & file);

		//! Load a MeshShaderObject from the given file.
		RENDERINGAPI static ShaderObjectInfo loadMesh(const Util::FileName & file);

		//! Load a TaskShaderObject from the given file.
		RENDERINGAPI static ShaderObjectInfo loadTask(const Util::FileName & file);
	
		const Util::FileName & getFileName() const { return filename; }
	private:
		ShaderObjectInfo& setFileName(const Util::FileName & f) { filename=f; return *this; }
};

}

#endif // RENDERING_SHADER_SHADEROBJECTINFO_H



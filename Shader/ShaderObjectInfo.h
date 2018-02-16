/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_SHADER_SHADEROBJECTINFO_H
#define RENDERING_SHADER_SHADEROBJECTINFO_H

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

// Forward declarations
namespace Util {
class FileName;
}
namespace Rendering {

/**
 * Storage for shader type and shader code. There is no GL handle stored in
 * here. When compiling an object, the handle is returned and has to be stored
 * by the caller.
 */
class ShaderObjectInfo {
	private:
		uint32_t type;
		std::string code;
		std::string defines;

		ShaderObjectInfo(uint32_t _type, std::string _code) :
			type(_type), code(std::move(_code)) {
		}

	public:
		const std::string & getCode() const {
			return code;
		}
		uint32_t getType() const {
			return type;
		}
		ShaderObjectInfo& addDefine(const std::string& key, const std::string& value="") {
			defines += "#define " + key + " " + value + "\n";
			return *this;
		}

		/**
		 * Use GL to compile the source stored in this shader object.
		 * 
		 * @return Handle of the compiled GL shader, or @c 0 in case of an
		 * error
		 */
		uint32_t compile() const;

		/**
		 * Create a VertexShaderObject from the given code
		 * 
		 * @note Inserts "#define SG_VERTEX_SHADER" before the first line.
		 */
		static ShaderObjectInfo createVertex(const std::string & code);

		/**
		 * Create a FragmentShaderObject from the given code
		 * 
		 * @note Inserts "#define SG_FRAGMENT_SHADER" before the first line.
		 */
		static ShaderObjectInfo createFragment(const std::string & code);

		/**
		 * Create a GeometryShaderObject from the given code
		 * 
		 * @note Inserts "#define SG_GEOMETRY_SHADER" before the first line.
		 */
		static ShaderObjectInfo createGeometry(const std::string & code);

		/**
		 * Create a ComputeShaderObject from the given code
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
};

}

#endif // RENDERING_SHADER_SHADEROBJECTINFO_H



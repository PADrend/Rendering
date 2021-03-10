/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2013 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "ShaderObjectInfo.h"
#include "../GLHeader.h"
#include "../Helper.h"
#include <Util/IO/FileUtils.h>
#include <Util/IO/FileLocator.h>
#include <Util/StringUtils.h>
#include <Util/Macros.h>
#include <cstddef>
#include <vector>
#include <regex>
#include <sstream>
#include <iostream>

namespace Rendering {
	
const uint32_t ShaderObjectInfo::SHADER_STAGE_VERTEX = GL_VERTEX_SHADER;
const uint32_t ShaderObjectInfo::SHADER_STAGE_FRAGMENT = GL_FRAGMENT_SHADER;
const uint32_t ShaderObjectInfo::SHADER_STAGE_GEOMETRY = GL_GEOMETRY_SHADER;
const uint32_t ShaderObjectInfo::SHADER_STAGE_TESS_CONTROL = GL_TESS_CONTROL_SHADER;
const uint32_t ShaderObjectInfo::SHADER_STAGE_TESS_EVALUATION = GL_TESS_EVALUATION_SHADER;
const uint32_t ShaderObjectInfo::SHADER_STAGE_COMPUTE = GL_COMPUTE_SHADER;
const uint32_t ShaderObjectInfo::SHADER_STAGE_TASK = GL_TASK_SHADER_NV;
const uint32_t ShaderObjectInfo::SHADER_STAGE_MESH = GL_MESH_SHADER_NV;

static void printShaderInfoLog(uint32_t obj, const std::string & code, const Util::FileName& file) {
	GLint infoLogLength = 0;
	GET_GL_ERROR();
	glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &infoLogLength);
	GET_GL_ERROR();
	if (infoLogLength > 1) {
		GLsizei charsWritten = 0;
		std::vector<GLchar> infoLog(static_cast<std::size_t>(infoLogLength), '\0');
		glGetShaderInfoLog(obj, static_cast<GLsizei>(infoLog.size()), &charsWritten, infoLog.data());
		std::string s(infoLog.data(), static_cast<std::size_t>(charsWritten));
		// Skip "Everything ok" messages from AMD-drivers.
		if(s.find("successfully") == std::string::npos && 
			s.find("shader(s) linked.") == std::string::npos && 
			s.find("No errors.") == std::string::npos) {
			if(file.empty())
				WARN(std::string("Shader compile error:\n") + s + "\nShader code:\n" + code);
			else
				WARN(std::string("Shader compile error:\n") + s + "\nin shader file: " + file.toShortString() + "\n");
		}
	}
	GET_GL_ERROR();
}

static bool resolveIncludes(std::string & code, const Util::FileName & f) {
	Util::FileLocator locator;
	locator.addSearchPath(f.getDir());
	static std::regex includePattern(R"(\s*#include\s+\"(.*)\"\s*)");
	static std::regex commentStartPattern(R"(.*\/\*.*)");
	static std::regex commentEndPattern(R"(.*\*\/.*)");
	std::smatch match;
	std::ostringstream newCode;
	std::istringstream codeStream(code);
	std::string line;
	uint32_t lineCount = 0;
	bool isCommentBlock = false;
	while(std::getline(codeStream, line)) {
		++lineCount;
		if(std::regex_match(line, commentStartPattern)) {
			isCommentBlock = true;
			newCode << line << std::endl;
		} else if(std::regex_match(line, commentEndPattern)) {
			isCommentBlock = false;
			newCode << line << std::endl;
		} else if(!isCommentBlock && std::regex_match(line, match, includePattern)) {
			auto result = locator.locateFile(Util::FileName(match[1]));
			if(!result.first) {
				std::stringstream ss;
				ss << "Shader parser error in file " << f.toShortString() << "(" << lineCount << "): Could not find include file '" << match[1] << "'" << std::endl;
				WARN(ss.str());
				return false;
			}
			std::string includedCode = Util::FileUtils::getParsedFileContents(result.second);
			if(!resolveIncludes(includedCode, result.second))
				return false;
			++lineCount;
			newCode << "#line 1" << std::endl;
			newCode << includedCode << std::endl;
			newCode << "#line " << lineCount << std::endl;
		} else {
			newCode << line << std::endl;
		}
	}
	code = newCode.str();
	return true;
}

uint32_t ShaderObjectInfo::compile() const {
	GLuint handle = glCreateShader(getType());
	// This postfix assures that the file not be empty even if everything is cut out due to an ifdef-commando.
	// Files without content do not compile with AMD cards.
	std::string strCode = getCode() + "\nvoid _();\n";
	std::string header;
	if(!resolveIncludes(strCode, filename))
		return 0;

	// prepend a "#define SG_shaderType" or insert it after the initial "#version..." line.
	if(getType() == GL_FRAGMENT_SHADER) {
		header = "#define SG_FRAGMENT_SHADER\n";
#ifdef LIB_GL
	} else if(getType() == GL_GEOMETRY_SHADER) {
		header = "#define SG_GEOMETRY_SHADER\n";
	} else if(getType() == GL_COMPUTE_SHADER) {
		header = "#define SG_COMPUTE_SHADER\n";
	} else if(getType() == GL_MESH_SHADER_NV) {
		header = "#define SG_MESH_SHADER\n#extension GL_NV_mesh_shader : require\n";
	} else if(getType() == GL_TASK_SHADER_NV) {
		header = "#define SG_TASK_SHADER\n#extension GL_NV_mesh_shader : require\n";
#endif /* LIB_GL */
	} else if(getType() == GL_VERTEX_SHADER) {
		header = "#define SG_VERTEX_SHADER\n";
	}
	header += defines;
	static const std::string versionPrefix = "#version";
	if(strCode.compare(0, versionPrefix.length(), versionPrefix) == 0) {
		header += "#line 2\n";
		strCode.replace(strCode.find('\n') + 1, 0, header);
	} else {
		header += "#line 1\n";
		strCode = header + strCode;
	}

	const char * str = strCode.c_str();
	glShaderSource(handle, 1, &str, nullptr);
	glCompileShader(handle);
	GET_GL_ERROR();
	GLint compileStatus;
	glGetShaderiv(handle, GL_COMPILE_STATUS, &compileStatus);
	if(compileStatus == GL_FALSE) {
		printShaderInfoLog(handle, strCode, filename);
		GET_GL_ERROR();
		glDeleteShader(handle);
		return 0;
	}
	return handle;
}

ShaderObjectInfo ShaderObjectInfo::createVertex(const std::string & code) {
	return ShaderObjectInfo(GL_VERTEX_SHADER, code);
}

ShaderObjectInfo ShaderObjectInfo::createFragment(const std::string & code) {
#ifdef LIB_GLESv2
	if(code.find("precision") == std::string::npos) {
		const std::string modifiedCode(std::string("precision mediump float;\n") + code);
		return ShaderObjectInfo(GL_FRAGMENT_SHADER, modifiedCode);
	}
#endif /* LIB_GLESv2 */
	return ShaderObjectInfo(GL_FRAGMENT_SHADER, code);
}

ShaderObjectInfo ShaderObjectInfo::createGeometry(const std::string & code) {
#if defined (LIB_GL) && defined (GL_ARB_geometry_shader4)
	return ShaderObjectInfo(GL_GEOMETRY_SHADER_ARB, code);
#else /* defined(GL_ARB_geometry_shader4) */
	throw std::logic_error("No support for GL_ARB_geometry_shader4.");
#endif /* defined(GL_ARB_geometry_shader4) */
}

ShaderObjectInfo ShaderObjectInfo::createCompute(const std::string & code) {
#if defined (LIB_GL) && defined (GL_ARB_compute_shader)
	return ShaderObjectInfo(GL_COMPUTE_SHADER, code);
#else /* defined(GL_ARB_compute_shader) */
	throw std::logic_error("No support for GL_ARB_compute_shader.");
#endif /* defined(GL_ARB_compute_shader) */
}

ShaderObjectInfo ShaderObjectInfo::createMesh(const std::string & code) {
#if defined (LIB_GL) && defined (GL_NV_mesh_shader)
	return ShaderObjectInfo(SHADER_STAGE_MESH, code);
#else /* defined(GL_ARB_compute_shader) */
	throw std::logic_error("No support for GL_NV_mesh_shader.");
#endif /* defined(GL_ARB_compute_shader) */
}

ShaderObjectInfo ShaderObjectInfo::createTask(const std::string & code) {
#if defined (LIB_GL) && defined (GL_NV_mesh_shader)
	return ShaderObjectInfo(SHADER_STAGE_TASK, code);
#else /* defined(GL_ARB_compute_shader) */
	throw std::logic_error("No support for GL_NV_mesh_shader.");
#endif /* defined(GL_ARB_compute_shader) */
}

ShaderObjectInfo ShaderObjectInfo::loadVertex(const Util::FileName & file) {
	return createVertex(Util::FileUtils::getParsedFileContents(file)).setFileName(file);
}

ShaderObjectInfo ShaderObjectInfo::loadFragment(const Util::FileName & file) {
	return createFragment(Util::FileUtils::getParsedFileContents(file)).setFileName(file);
}

ShaderObjectInfo ShaderObjectInfo::loadGeometry(const Util::FileName & file) {
	return createGeometry(Util::FileUtils::getParsedFileContents(file)).setFileName(file);
}

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

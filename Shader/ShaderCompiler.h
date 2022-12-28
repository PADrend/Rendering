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
#ifndef RENDERING_SHADERCOMPILER_H_
#define RENDERING_SHADERCOMPILER_H_

#include <Util/ReferenceCounter.h>
#include <Util/TypeNameMacro.h>
#include <Util/IO/FileLocator.h>

#include <nvrhi/nvrhi.h>

#include <vector>

namespace Rendering {

class ShaderCompilerGLSL : public Util::ReferenceCounter<ShaderCompilerGLSL> {
	PROVIDES_TYPE_NAME(ShaderCompilerGLSL)
public:
	ShaderCompilerGLSL();
	~ShaderCompilerGLSL() = default;

	bool compile(const Util::FileName& file, nvrhi::ShaderType type, std::vector<uint32_t>& outByteCode);
	bool compile(const std::string& source, nvrhi::ShaderType type, std::vector<uint32_t>& outByteCode);

	bool compileLibrary(const Util::FileName& file, std::vector<uint32_t>& outByteCode);
	bool compileLibrary(const std::string& source, std::vector<uint32_t>& outByteCode);

	void addSearchPath(const std::string& path) { locator.addSearchPath(path); }
private:
	Util::FileLocator locator;
};

} // Rendering

#endif // RENDERING_SHADERCOMPILER_H_
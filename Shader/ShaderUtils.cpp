/*
	This file is part of the Rendering library.
	Copyright (C) 2007-2012 Benjamin Eikel <benjamin@eikel.org>
	Copyright (C) 2007-2012 Claudius Jähn <claudius@uni-paderborn.de>
	Copyright (C) 2007-2012 Ralf Petring <ralf@petring.net>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the 
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#include "ShaderUtils.h"
#include "Shader.h"
#include <Util/References.h>

namespace Rendering {
namespace ShaderUtils {

Util::Reference<Shader> createNormalToColorShader() {
	const std::string vertexProgram(
R"***(#version 110
uniform mat4 sg_cameraInverseMatrix;
uniform mat4 sg_modelViewMatrix;
varying vec3 normal;

void main() {
	normal = normalize((sg_cameraInverseMatrix * sg_modelViewMatrix * vec4(gl_Normal, 0.0)).xyz);
	gl_Position = ftransform();
}
)***");
	const std::string fragmentProgram(
R"***(#version 110
varying vec3 normal;

void main() {
	gl_FragColor = vec4(0.5 * normalize(normal) + 0.5, 1.0);
}
)***");
	return Shader::createShader(vertexProgram, fragmentProgram, Shader::USE_GL | Shader::USE_UNIFORMS);
}

}
}

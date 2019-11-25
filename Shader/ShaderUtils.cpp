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
#include <string>

namespace Rendering {
namespace ShaderUtils {

Util::Reference<Shader> createNormalToColorShader() {
	const std::string vertexProgram(
R"***(#version 110
uniform mat4 sg_matrix_cameraToWorld;
uniform mat4 sg_matrix_modelToCamera;
varying vec3 normal;

void main() {
	normal = normalize((sg_matrix_cameraToWorld * sg_matrix_modelToCamera * vec4(gl_Normal, 0.0)).xyz);
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

Util::Reference<Shader> createDefaultShader() {
	const std::string vertexProgram(
R"***(#version 130
in vec3 sg_Position;
in vec4 sg_Color;
uniform mat4 sg_matrix_modelToClipping;
out vec4 vs_color;
void main() {
	vs_color = sg_Color;
	gl_Position = sg_matrix_modelToClipping * vec4(sg_Position, 1.0);
}
)***");
	const std::string fragmentProgram(
R"***(#version 130
in vec4 vs_color;
out vec4 fs_color;
void main() {
	fs_color = vs_color;
}
)***");
	return Shader::createShader(vertexProgram, fragmentProgram, Shader::USE_GL | Shader::USE_UNIFORMS);
}

}
}

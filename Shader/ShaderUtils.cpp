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
R"***(#version 420
#extension GL_ARB_shader_draw_parameters : require
layout(location=0) in vec3 sg_Position;
layout(std140, binding=0, row_major) uniform FrameData {
  mat4 sg_matrix_worldToCamera;
  mat4 sg_matrix_cameraToWorld;
  mat4 sg_matrix_cameraToClipping;
  mat4 sg_matrix_clippingToCamera;
  vec4 sg_viewport;
};
struct Object {
  mat4 sg_matrix_modelToCamera;
  float sg_pointSize;
  uint materialId;
  uint lightSetId;
  uint drawId;
};
layout(std140, binding=4, row_major) uniform ObjectData {
  Object objects[512];
};
out vec3 normal
void main() {
	gl_Position = sg_matrix_cameraToClipping * objects[gl_BaseInstanceARB].sg_matrix_modelToCamera * vec4(sg_Position, 1.0);
}
)***");
	const std::string fragmentProgram(
R"***(#version 420
in vec3 normal;
out vec4 fragColor;

void main() {
	fragColor = vec4(0.5 * normalize(normal) + 0.5, 1.0);
}
)***");
	return Shader::createShader(vertexProgram, fragmentProgram);
}



Util::Reference<Shader> createPassThroughShader() {
	const std::string vertexProgram(
R"***(#version 420
#extension GL_ARB_shader_draw_parameters : require
layout(location=0) in vec3 sg_Position;
layout(std140, binding=0, row_major) uniform FrameData {
  mat4 sg_matrix_worldToCamera;
  mat4 sg_matrix_cameraToWorld;
  mat4 sg_matrix_cameraToClipping;
  mat4 sg_matrix_clippingToCamera;
  vec4 sg_viewport;
};
struct Object {
  mat4 sg_matrix_modelToCamera;
  float sg_pointSize;
  uint materialId;
  uint lightSetId;
  uint drawId;
};
layout(std140, binding=4, row_major) uniform ObjectData {
  Object objects[512];
};
void main() {
	gl_Position = sg_matrix_cameraToClipping * objects[gl_BaseInstanceARB].sg_matrix_modelToCamera * vec4(sg_Position, 1.0);
}
)***");
	Util::Reference<Shader> shader = Shader::createShader();
	shader->attachShaderObject(ShaderObjectInfo::createVertex(vertexProgram));
	return shader;
}


}
}

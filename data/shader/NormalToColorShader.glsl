/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#version 450

#include "Materials.glsl"

#ifdef SG_VERTEX_SHADER
	layout(location = 0) in vec3 sg_Position;
	layout(location = 1) in vec3 sg_Normal;
	layout(location = 2) in vec4 sg_Color;
	layout(location = 3) in vec2 sg_TexCoord0;

	layout(push_constant) uniform ObjectBuffer {
		mat4 sg_matrix_modelToCamera;
	};

	layout(set=1,binding=0) uniform CameraBuffer {
		mat4 sg_matrix_cameraToClipping;
	};

	layout(location = 0) out vec4 normal;

	void main() {
		vec4 position = sg_matrix_modelToCamera * vec4(sg_Position, 1.0);
		normal = sg_matrix_modelToCamera * vec4(sg_Normal, 0.0);
		gl_Position = sg_matrix_cameraToClipping * position;
	}
#endif

#ifdef SG_FRAGMENT_SHADER
	layout(location = 0) in vec4 normal;
	layout(location = 0) out vec4 finalColor;
	
	void main() {
		finalColor = vec4(0.5 * normalize(normal / normal.w) + 0.5, 1.0);
	}
#endif

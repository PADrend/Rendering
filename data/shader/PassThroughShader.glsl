/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#version 450

#ifdef SG_VERTEX_SHADER
	layout(location = 0) in vec3 sg_Position;

	layout(push_constant) uniform ObjectBuffer {
		mat4 sg_matrix_modelToCamera;
	};

	layout(set=1,binding=0) uniform CameraBuffer {
		mat4 sg_matrix_cameraToClipping;
	};

	void main() {
		vec4 position = sg_matrix_modelToCamera * vec4(sg_Position, 1.0);		
		gl_Position = sg_matrix_cameraToClipping * position;
	}
#endif

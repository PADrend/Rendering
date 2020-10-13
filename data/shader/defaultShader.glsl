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

	layout(push_constant) uniform ObjectBuffer {
		mat4 sg_matrix_modelToCamera;
	};

	layout(set=0,binding=0) uniform CameraBuffer {
		mat4 sg_matrix_cameraToClipping;
	};

	layout(location = 0) out VertexData vertex;

	void main() {
		vertex.position = (sg_matrix_modelToCamera * vec4(sg_Position, 1.0)).xyz;
		vertex.normal = (sg_matrix_modelToCamera * vec4(sg_Normal, 0.0)).xyz;
		vertex.color = sg_Color;
		
		gl_Position = sg_matrix_cameraToClipping * vec4(vertex.position, 1.0);
		gl_Position.y = -gl_Position.y; // Vulkan uses right hand NDC
	}
#endif

#ifdef SG_FRAGMENT_SHADER
	layout(location = 0) in VertexData vertex;
	layout(location = 0) out vec4 finalColor;

	layout(set=1,binding=0) uniform LightBuffer {
		LightData sg_Light[8];
		uint sg_lightCount;
	};

	layout(set=2,binding=0) uniform MaterialBuffer {
		MaterialData sg_Material;
	};
	
	void main() {
		SurfaceSample surface = initSurface(vertex, sg_Material);
		
		vec4 color = vec4(0.0,0.0,0.0,1.0);
		// Reflectance equation: L_o = integral_Omega{ f(L,V) * L_i(L) * (N dot V) } dw_i
		for(uint i=0; i<sg_lightCount; ++i) {
			color.rgb += evalMaterial(surface, sg_Light[i]);
		}
		color.rgb += sg_Material.ambient.rgb;
		color.rgb += sg_Material.emission.rgb * sg_Material.emission.a;
		color.a = sg_Material.diffuse.a;

		finalColor = color;
	}
#endif

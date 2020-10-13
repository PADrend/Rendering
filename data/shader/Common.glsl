/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_SHADER_COMMON_GLSL_
#define RENDERING_SHADER_COMMON_GLSL_

struct VertexData {
	vec3 position;
	vec3 normal;
	vec4 color;
	vec2 texCoord;
};

struct SurfaceSample {
	vec3 position;
	vec3 normal;
	vec4 diffuse;
	vec3 specular;
	float roughness;
	float ggxAlpha;
	float NdotV;
};

struct LightSample {
	vec4 intensity;
	float NdotL;
	float NdotH;
	float LdotH;
};

#endif /* end of include guard: RENDERING_SHADER_COMMON_GLSL_ */
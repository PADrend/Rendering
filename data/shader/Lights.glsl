/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_SHADER_LIGHTS_GLSL_
#define RENDERING_SHADER_LIGHTS_GLSL_

#include "Common.glsl"

#define DIRECTIONAL 1
#define POINT 2
#define SPOT 3

struct LightData {
	vec3 position;
	vec3 direction;
	vec4 intensity;
	float range;
	float cosConeAngle;
	uint type;
};

LightSample evalLight(in SurfaceSample surface, in LightData light) {
	LightSample ls;

	// for DIRECTIONAL lights
	vec3 L = -light.direction;
	float falloff = 1.0;
			
	// for POINT & SPOT lights
	if(light.type != DIRECTIONAL) { 
		L = light.position - surface.position;
		float dist = length(L); 
		L = normalize(L);
		falloff = 1 / ((0.01 * 0.01) + (dist * dist)); // The 0.01 is to avoid infs when the light source is close to the shading point
	}

	// for SPOT lights
	if(light.type == SPOT) {
		float cosTheta = dot(L, -light.direction); // cos of angle of light orientation
		if(cosTheta < light.cosConeAngle) {
			falloff = 0.0;
		}
		// TODO: calculate penumbra
	}
	
	vec3 H = normalize(normalize(-surface.position) + L);
	ls.NdotL = dot(surface.normal, L);
	ls.NdotH = dot(surface.normal, H);
	ls.LdotH = dot(L, H);
	ls.intensity = light.intensity * falloff;
	return ls;
}

#endif /* end of include guard: RENDERING_SHADER_LIGHTS_GLSL_ */
/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_SHADER_MATERIALS_GLSL_
#define RENDERING_SHADER_MATERIALS_GLSL_

#include "Lights.glsl"
#include "BRDF.glsl"

struct MaterialData {
	vec4 ambient, diffuse, specular, emission;
};

SurfaceSample initSurface(in VertexData vertex, in MaterialData material) {
	SurfaceSample surface;
	surface.position = vertex.position;
	surface.normal = normalize(vertex.normal);
	surface.diffuse.rgb = vertex.color.rgb * material.diffuse.rgb;
	surface.diffuse.a = material.diffuse.a;
	surface.specular = material.specular.rgb;
	surface.NdotV = clamp(dot(surface.normal, -surface.position), 0.0, 1.0);
	surface.roughness = max(0.08, 1.0 - material.specular.a);
	surface.ggxAlpha = surface.roughness * surface.roughness;
	return surface;
}

// f(L,V) * L_i(L) * (N dot V) = (f_diff(L,V) + f_spec(L,V)) * L_i(L) * (N dot V)
vec3 evalMaterial(in SurfaceSample surface, in LightData light) {
	vec3 color = vec3(0);
	LightSample ls = evalLight(surface, light);

	// If the light doesn't hit the surface or we are viewing the surface from the back, return
	if (ls.NdotL <= 0) return color;

	// Calculate the diffuse term
	vec3 diffuseBrdf = surface.diffuse.rgb * M_PI_INV; // lambertian brdf
	color = diffuseBrdf * ls.intensity.rgb * ls.NdotL;

	// Calculate the specular term
	vec3 specularBrdf = evalSpecularBRDF(surface, ls);
	color += specularBrdf * ls.intensity.rgb * ls.NdotL;

	return color;
}

#endif /* end of include guard: RENDERING_SHADER_MATERIALS_GLSL_ */
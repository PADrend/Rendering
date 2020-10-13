/*
	This file is part of the Rendering library.
	Copyright (C) 2020 Sascha Brandt <sascha@brandt.graphics>
	
	This library is subject to the terms of the Mozilla Public License, v. 2.0.
	You should have received a copy of the MPL along with this library; see the
	file LICENSE. If not, you can obtain one at http://mozilla.org/MPL/2.0/.
*/
#ifndef RENDERING_SHADER_BRDF_GLSL_
#define RENDERING_SHADER_BRDF_GLSL_

#include "Common.glsl"

#define M_PI 3.1415926535897932384626433832795
#define M_PI_INV 0.3183098861837906715377675267450

vec3 fresnelSchlick(vec3 specular, float LdotH) {
	return specular + (1 - specular) * pow(1 - LdotH, 5);
}

// Normal distribution function: D_GGX(H) = alpha^2 / ( PI * ((N dot H)^2 * (alpha^2-1) + 1)^2 )
float evalGGX(float alpha, float NdotH) {
	float a2 = alpha * alpha;
	float d = ((NdotH * a2 - NdotH) * NdotH) + 1;
	return a2 * M_PI_INV / (d * d);
}

// Visibility term: V(L,V) = G(L,V,H) / 4 (N dot L)(N dot V) (G = geometry term)
float evalSmithGGX(float NdotL, float NdotV, float alpha) {
	// Smith: G(L,V,H) = G_1(V) * G_1(L)
	// GGX: G_GGX(V) = 2(N dot V) / ( (N dot V) + sqrt(alpha^2 + (1-alpha^2)(N dot V)^2) )
	// Combined: V(L,V) = 1 / (( (N dot V) + sqrt(alpha^2 + (1-alpha^2)(N dot V)^2) ) * ( (N dot L) + sqrt(alpha^2 + (1-alpha^2)(N dot L)^2) ))
	float a2 = alpha*alpha;
	float G_GGX_V = NdotV + sqrt( (NdotV - NdotV * a2) * NdotV + a2 );
	float G_GGX_L = NdotL + sqrt( (NdotL - NdotL * a2) * NdotL + a2 );
	return 1 / ( G_GGX_V * G_GGX_L );
}

// General microfacet BRDF: f(L,V) = F(L,H) * V(L,V) * D(H)
vec3 evalSpecularBRDF(in SurfaceSample surface, in LightSample light) {
	float D = evalGGX(surface.ggxAlpha, light.NdotH); // normal distribution
	float V = evalSmithGGX(light.NdotL, surface.NdotV, surface.ggxAlpha); // visibility term
	vec3 F = fresnelSchlick(surface.specular, max(0, light.LdotH)); // fresnel
	return D * V * F;
}

#endif /* end of include guard: RENDERING_SHADER_BRDF_GLSL_ */
#version 450

#define M_PI 3.1415926535897932384626433832795

#define DIRECTIONAL 1
#define POINT 2
#define SPOT 3

struct SurfaceData {
	vec3 position_cs, normal_cs;
	vec4 diffuse;
	float NdotV;
};

struct LightData {
	vec3 position;
	vec3 direction;
	vec4 intensity;
	float range;
	float cosConeAngle;
	uint type;
};

struct LightSample {
	vec4 diffuse;
	vec4 specular;
	float NdotL;
};

struct MaterialData {
	vec4 ambient, diffuse, specular, emission;
};

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

	layout(set=1,binding=0) uniform LightBuffer {
		LightData sg_Light[8];
		uint sg_lightCount;
	};

	layout(set=2,binding=0) uniform MaterialBuffer {
		MaterialData sg_Material;
	};

	/*float evalGGX(float ggxAlpha, float NdotH) {
		float a2 = ggxAlpha * ggxAlpha;
		float d = ((NdotH * a2 - NdotH) * NdotH + 1);
		return a2 / (d * d);
	}

	float evalSmithGGX(float NdotL, float NdotV, float ggxAlpha) {
		// Optimized version of Smith, already taking into account the division by (4 * NdotV)
		float a2 = ggxAlpha * ggxAlpha;
		// `NdotV *` and `NdotL *` are inversed. It's not a mistake.
		float ggxv = NdotL * sqrt((-NdotV * a2 + NdotV) * NdotV + a2);
		float ggxl = NdotV * sqrt((-NdotL * a2 + NdotL) * NdotL + a2);
		return 0.5f / (ggxv + ggxl);
	}

	float3 evalSpecularBrdf(ShadingData sd, LightSample ls)
	{
		float ggxAlpha = sd.ggxAlpha;
		
		float D = evalGGX(ggxAlpha, ls.NdotH);
		float G = evalSmithGGX(ls.NdotL, sd.NdotV, ggxAlpha);
		float3 F = fresnelSchlick(sd.specular, 1, max(0, ls.LdotH));
		return D * G * F * M_INV_PI;
	}
		
	LightSample evalDirectionalLight(in LightData light, in float3 surfacePosW) {
		LightSample ls;
		ls.diffuse = light.intensity;
		ls.specular = light.intensity;
		ls.L = -normalize(light.dirW);
		float dist = length(surfacePosW - light.posW);
		ls.posW = surfacePosW - light.dirW * dist;
		return ls;
	}

	LightSample evalPointLight(in LightData light, in float3 surfacePosW) {
		LightSample ls;
		ls.posW = light.posW;
		ls.L = light.posW - surfacePosW;
		// Avoid NaN
		float distSquared = dot(ls.L, ls.L);
		ls.distance = (distSquared > 1e-5f) ? length(ls.L) : 0;
		ls.L = (distSquared > 1e-5f) ? normalize(ls.L) : 0;

		// Calculate the falloff
		float falloff = getDistanceFalloff(distSquared);

		// Calculate the falloff for spot-lights
		float cosTheta = -dot(ls.L, light.dirW); // cos of angle of light orientation
		if (cosTheta < light.cosOpeningAngle)
		{
			falloff = 0;
		}
		else if (light.penumbraAngle > 0)
		{
			float deltaAngle = light.openingAngle - acos(cosTheta);
			falloff *= saturate((deltaAngle - light.penumbraAngle) / light.penumbraAngle);
		}
		ls.diffuse = light.intensity * falloff;
		ls.specular = ls.diffuse;
		return ls;
	}*/

	LightSample evalLight(in SurfaceData surface, in LightData light) {
		LightSample ls;

		// for DIRECTIONAL lights
		vec3 pixToLight = -light.direction;
		float falloff = 1.0;
				
		// for POINT & SPOT lights
		if(light.type != DIRECTIONAL) { 
			pixToLight = light.position - surface.position_cs;
			float distance = length(pixToLight); 
			pixToLight = normalize(pixToLight);
			falloff = 1 / ((0.01 * 0.01) + (distance * distance)); // The 0.01 is to avoid infs when the light source is close to the shading point
		}

		// for SPOT lights
		if(light.type == SPOT) {
			float cosTheta = dot(pixToLight, -light.direction); // cos of angle of light orientation
			if(cosTheta < light.cosConeAngle) {
				falloff = 0.0;
			}
		}
		
		ls.NdotL = max(0.0, dot(surface.normal_cs, pixToLight));
		ls.diffuse = light.intensity * falloff;
		ls.specular = ls.diffuse;
		return ls;
	}

	vec4 evalMaterial(in SurfaceData surface, in MaterialData mat, in LightData light) {
		vec4 color = vec4(0);
		LightSample ls = evalLight(surface, light);

		// If the light doesn't hit the surface or we are viewing the surface from the back, return
		if (ls.NdotL <= 0) return color;

		// Calculate the diffuse term
		vec3 diffuseBrdf = mat.diffuse.rgb * (1 / M_PI); // lambertian brdf
		color.rgb = ls.diffuse.rgb * diffuseBrdf * ls.NdotL;

		// Calculate the specular term
		/*float ggxAlpha = 1.0;
		float D = evalGGX(ggxAlpha, ls.NdotH);
		float G = evalSmithGGX(ls.NdotL, sd.NdotV, ggxAlpha);
		float3 F = fresnelSchlick(sd.specular, 1, max(0, ls.LdotH));
		float specularBrdf = D * G * F * M_INV_PI;
		color.rgb += ls.specular * specularBrdf * ls.NdotL;*/

		return color;
	}

	layout(location = 0) out vec4 fragColor;

	void main() {
		SurfaceData surface;
		surface.position_cs = (sg_matrix_modelToCamera * vec4(sg_Position, 1.0)).xyz;
		surface.normal_cs = (sg_matrix_modelToCamera * vec4(sg_Normal, 0.0)).xyz;
		surface.diffuse = sg_Color;
		surface.NdotV = clamp(dot(-surface.position_cs, surface.normal_cs), 0.0, 1.0);
		vec4 color = vec4(0.0,0.0,0.0,1.0);
		for(uint i=0; i<sg_lightCount; ++i) {
			color.rgb += evalMaterial(surface, sg_Material, sg_Light[i]).rgb;
		}
		color.rgb += sg_Material.ambient.rgb;
		color.rgb += sg_Material.emission.rgb * sg_Material.emission.a;
		color.a = sg_Material.diffuse.a;
		fragColor = color;
		gl_Position = sg_matrix_cameraToClipping * vec4(surface.position_cs, 1.0);
		gl_Position.y = -gl_Position.y; // Vulkan uses right hand NDC
	}
#endif

#ifdef SG_FRAGMENT_SHADER
	layout(location = 0) in vec4 fragColor;
	layout(location = 0) out vec4 outColor;

	void main() {
		outColor = fragColor;
	}
#endif

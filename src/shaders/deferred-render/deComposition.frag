#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 0) uniform sampler2D samplerPosition;
layout(set = 0, binding = 1) uniform sampler2D samplerNormal;
layout(set = 0, binding = 2) uniform sampler2D samplerAlbedo;
layout(set = 0, binding = 3) uniform sampler2DArray samplerShadowMap;

#define LIGHT_COUNT 2
#define AMBIENT_COLOR .5

struct Light 
{
	float shininess; /* exponent value */
	vec3 position; /* position of light */
	vec3 ambient; /* scene color */
	vec3 albedo; /* base color of light */
	vec3 specular; /* reflectivity of the light */
	mat4 viewMatrix; /* projects a point to the light's POV */
};

layout(set = 1, binding = 0) uniform UBO
{
	Light lights[LIGHT_COUNT];
	vec3 viewPosition; /* position of the camera (for view direction calculation) */
} ubo;


float ShadowSampling(vec4 fragPos)
{
	float shadow = 1.0;

	for (int layer = 0; layer < LIGHT_COUNT; ++layer)
	{
		vec4 shadowClip = ubo.lights[layer].viewMatrix * fragPos;
		vec3 shadowCoord = shadowClip.xyz / shadowClip.w;

		shadowCoord.xy = shadowCoord.xy * 0.5 + 0.5;

		if (shadowCoord.z >= 0.0 && shadowCoord.z <= 1.0)
		{
		    float dist = texture(samplerShadowMap, vec3(shadowCoord.xy, layer)).r;

		    if (dist < shadowCoord.z)
			{
				shadow *= AMBIENT_COLOR;
			}
		}
	}

    return shadow;
}



void main()
{
	vec3 position = texture(samplerPosition, inUV).rgb;
	vec3 normal = texture(samplerNormal, inUV).rgb;
	vec3 albedo = texture(samplerAlbedo, inUV).rgb;

	fragColor.rgb = vec3(AMBIENT_COLOR) * vec3(albedo);

	float shadowFactor = ShadowSampling(vec4(position, 1));

	for (int i = 0; i < LIGHT_COUNT; ++i)
	{
		vec3 lightDir = normalize(ubo.lights[i].position - position);
		
		vec3 viewDir = normalize(ubo.viewPosition-position);
		vec3 halfwayDir = normalize(lightDir + viewDir);

		float specular = pow(max(dot(normal, halfwayDir), 0.0), ubo.lights[i].shininess);
		
		float diffuse = max(dot(normal, lightDir), 0.0f);

		fragColor.rgb += (diffuse + specular) * vec3(albedo); 
	}

	fragColor.rgb *= vec3(shadowFactor);
	fragColor.a = 1.f;
}
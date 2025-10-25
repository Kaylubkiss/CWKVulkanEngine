#version 450


layout(binding = 1) uniform sampler2D samplerPosition;
layout(binding = 2) uniform sampler2D samplerNormal;
layout(binding = 3) uniform sampler2D samplerAlbedo;
layout(binding = 4) uniform sampler2DArray samplerShadowMap;

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 fragColor;

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

layout(binding = 0) uniform UBO
{
	Light lights[LIGHT_COUNT];
	vec3 viewPosition; /* position of the camera (for view direction calculation) */

} ubo;

float ShadowSampling(vec4 fragPos)
{
	float factor = 1.f;

	for (int layer = 0; layer < LIGHT_COUNT; ++layer)
	{
		vec4 shadowClip = ubo.lights[layer].viewMatrix * fragPos;
		vec4 shadowCoord = shadowClip / shadowClip.w; //persp divide
		shadowCoord.st = shadowCoord.st * 0.5 + 0.5; //now in texture space

		if (shadowCoord.z > -1.f && shadowCoord.z < 1.f)
		{
			float dist = texture(samplerShadowMap, vec3(shadowCoord.st, layer)).r;

			if (shadowCoord.w > 0.f && dist < shadowCoord.z)
			{
				factor *= .25f;
			}
		}
	}

	return factor;
}



void main()
{
	vec3 position = texture(samplerPosition, inUV).rgb;
	vec3 normal = texture(samplerNormal, inUV).rgb;
	vec3 albedo = texture(samplerAlbedo, inUV).rgb;

	fragColor.rgb = AMBIENT_COLOR * albedo;

	float shadowFactor = ShadowSampling(vec4(position, 1));

	for (int i = 0; i < LIGHT_COUNT; ++i)
	{
		vec3 lightDir = normalize(ubo.lights[i].position - position);
		
		vec3 viewDir = normalize(ubo.viewPosition-position);
		vec3 halfwayDir = normalize(lightDir + viewDir);

		float specular = pow(max(dot(normal, halfwayDir), 0.0), ubo.lights[i].shininess);
		
		float diffuse = max(dot(normal, lightDir), 0.0f);

		fragColor.rgb += (diffuse + specular) * albedo; 
	}

	fragColor.rgb *= shadowFactor; 

	fragColor.a = 1.f;
}
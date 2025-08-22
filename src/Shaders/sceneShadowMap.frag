#version 450

layout(location = 0) in vec3 worldNormal;
layout(location = 1) in vec3 lightDir;
layout(location = 2) in vec3 viewDir;
layout(location = 3) in vec4 shadowCoord;

layout(binding = 1) uniform sampler2D shadowMap;


float ShadowSampling(vec4 shadowCoord, vec2 texOffset)
{

	float shadow = 1.0;

	if (shadowCoord.z > -1.0 && shadowCoord.z < 1.0)
	{
		float dist = texture(shadowMap, shadowCoord.st + texOffset).r;
		if (dist < shadowCoord.z)
		{
			shadow = 0.0;
		}
	}

	return shadow;
}

layout (location = 0) out vec4 outColor;

void main()
{
	float shadow = shadowCoord.w > 0.0 ? ShadowSampling(shadowCoord / shadowCoord.w, vec2(0.0)) : 0.0;

	vec3 N = normalize(worldNormal);
	vec3 L = normalize(lightDir);
	vec3 color = vec3(1,1,1);

	float diff = max(dot(N, L), 0.0);
	vec3 diffuse = diff * color;
	
	outColor.rgb = shadow * diffuse;
	outColor.a = 1;
}
#version 450

layout(location = 0) in vec3 Normal;
layout(location = 1) in vec3 lightDir;
layout(location = 2) in vec3 viewDir;
layout(location = 3) in vec4 shadowCoord;

layout(binding = 1) uniform sampler2D shadowMap;

layout(std140, binding = 0) uniform UBO {
	//standard transformations.
	mat4 view;
	mat4 proj;

	//lightWorld
	mat4 depthVP;

	//camera
	vec3 camPos;

	
	vec3 lightPos; /* position of light */
	vec3 lightAmbient; /* scene color */
	vec3 lightAlbedo; /* base color of light */
	vec3 lightSpecular; /* reflectivity of the light */
	vec3 lightShininess; /* exponent value */


} ubo;


float ShadowSampling(vec3 shadowCoord, vec2 texOffset)
{
	shadowCoord = shadowCoord * 0.5 + 0.5;

	float closestDepth = texture(shadowMap, shadowCoord.xy).r;

	return closestDepth;

	float currentDepth = shadowCoord.z;

	return currentDepth > closestDepth ? 1.0 : 0.0;
}

layout (location = 0) out vec4 outColor;

void main()
{
	float shadow = ShadowSampling(shadowCoord.xyz / shadowCoord.w, vec2(0.0));

	vec3 N = normalize(Normal);
	vec3 L = normalize(lightDir);
	
	vec3 V = normalize(viewDir);
//	vec3 halfwayDir = normalize(L + V);

//	float spec = pow(max(dot(norm, halfwayDir), 0.0), ubo.lightShininess.x);
//	vec3 specular = spec * ubo.lightAlbedo;  
//	
	vec3 color = vec3(1,1,1);

	float diff = max(dot(N, L), 0.0);
	vec3 diffuse = diff * color;
	
	outColor.rgb = (ubo.lightAmbient + (1-shadow)) * diffuse;
	//outColor.rgb = vec3(shadow);
	outColor.a = 1;
}
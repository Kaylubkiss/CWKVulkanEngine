#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(set = 2, binding = 0) uniform sampler2D colorSampler;
layout(set = 2, binding = 1) uniform sampler2D normalSampler;
layout(set = 2, binding = 2) uniform sampler2D metallicRoughnessSampler;
layout(set = 2, binding = 3) uniform sampler2D ambientOcclusionSampler;

layout( location = 0 ) in vec4 inWorldPosition;
layout( location = 1 ) in vec4 inWorldNormal;
layout( location = 2 ) in vec2 inTexCoord;

layout( location = 0 ) out vec4 outPosition;
layout( location = 1 ) out vec4 outNormal;
layout( location = 2 ) out vec4 outAlbedo;
layout( location = 3 ) out vec4 outMetallicRoughness;
layout( location = 4 ) out vec4 outAmbientOcclusion;

void main()
{
	outPosition = inWorldPosition;

	vec4 normalMap = texture(normalSampler, inTexCoord);
	if (normalMap.x != 0 && normalMap.y != 0 && normalMap.z != 0)
	{
	   outNormal = normalMap;
	}
	else
	{
       outNormal = inWorldNormal;
    }

	outAlbedo = texture(colorSampler, inTexCoord);
	outMetallicRoughness = texture(metallicRoughnessSampler, inTexCoord);
	outAmbientOcclusion = texture(ambientOcclusionSampler, inTexCoord);
}
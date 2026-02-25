#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(set = 1, binding = 0) uniform sampler2D colorSampler;
layout(set = 1, binding = 1) uniform sampler2D metallicRoughnessSampler;

layout( location = 0 ) in vec4 inWorldPosition;
layout( location = 1 ) in vec4 inWorldNormal;
layout( location = 2 ) in vec2 inTexCoord;


layout( location = 0 ) out vec4 outPosition;
layout( location = 1 ) out vec4 outNormal;
layout( location = 2 ) out vec4 outAlbedo;
layout( location = 3) out vec4 outMetallicRoughness;

void main()
{
	outPosition = inWorldPosition;
	outNormal = vec4(normalize(vec3(inWorldNormal)), 1.f);
	outAlbedo = texture(colorSampler, inTexCoord);
	outMetallicRoughness = texture(metallicRoughnessSampler, inTexCoord);
}
#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(set = 1, binding = 0) uniform sampler2D colorSampler;

layout( location = 0 ) in vec4 inWorldPosition;
layout( location = 1 ) in vec4 inWorldNormal;
layout( location = 2 ) in vec2 inTexCoord;


layout( location = 0 ) out vec4 outPosition;
layout( location = 1 ) out vec4 outNormal;
layout( location = 2 ) out vec4 outAlbedo;

void main()
{
	outPosition = inWorldPosition;
	outNormal = inWorldNormal;
	outAlbedo = texture(colorSampler, inTexCoord);
}
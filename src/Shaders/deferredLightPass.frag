#version 450


layout(binding = 1) uniform sampler2D samplerPosition;
layout(binding = 2) uniform sampler2D samplerNormal;
layout(binding = 3) uniform sampler2D samplerAlbedo;

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 fragColor;

void main()
{
	fragColor.rgb = texture(samplerPosition, inUV).rgb;
	fragColor.a = 1.f;
}
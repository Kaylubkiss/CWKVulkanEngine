#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(set = 1, binding = 0) uniform sampler2D samplerSceneColor;

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 fragColor;

void main()
{
    fragColor = texture(samplerSceneColor, inUV);

    fragColor.rgb = fragColor.rgb / (fragColor.rgb + vec3(1.0));
    fragColor.rgb = pow(fragColor.rgb, vec3(1.0/2.2)); //convert to SRGB

    fragColor.a = 1.f;
}
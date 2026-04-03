#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(set = 2, binding = 0) uniform samplerCube samplerSkybox;

layout( location = 0 ) in vec3 inUVW;

layout( location = 0 ) out vec4 fragColor;

void main()
{
    fragColor = texture(samplerSkybox, inUVW);
}
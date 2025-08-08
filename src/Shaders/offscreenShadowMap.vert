#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) in vec3 aPos;

layout(binding = 0) uniform UBO
{
	mat4 depthVP;
} ubo;


layout(push_constant) uniform pc 
{
	mat4 modelMatrix;
};

void main()
{
	gl_Position = ubo.depthVP * modelMatrix * vec4(aPos, 1.0f);
}
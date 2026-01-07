#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(set = 0, binding = 0) uniform sceneUBO
{
	mat4 view;
	mat4 proj;
} camera;

layout (push_constant) uniform pc
{
	mat4 modelMatrix;
};

layout( location = 0 ) in vec3 aPos;
layout( location = 1 ) in vec3 aNorm;
layout( location = 2 ) in vec2 aUv;

layout( location = 0 ) out vec4 outWorldPosition;
layout( location = 1 ) out vec4 outWorldNormal;
layout( location = 2 ) out vec2 outTexCoord;
layout( location = 3 ) out vec4 outColor;

void main()
{
	gl_Position = camera.proj * camera.view * modelMatrix * vec4(aPos, 1);
	
	outWorldNormal = normalize(transpose(inverse(modelMatrix)) * vec4(aNorm, 0));
	outWorldPosition = modelMatrix * vec4(aPos, 1);
	outTexCoord = aUv;
}
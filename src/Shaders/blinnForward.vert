#version 450 
#extension GL_KHR_vulkan_glsl : enable


layout(binding = 0) uniform uTransformObject {
    mat4 view;
    mat4 proj;
	vec3 camPosition;
} ubo;


layout( location = 0 ) in vec3 aPos;
layout( location = 1 ) in vec3 aNorm;
layout( location = 2 ) in vec2 aUv;


layout (push_constant) uniform pc
{
	mat4 modelMatrix;
};

layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 Normal;
layout(location = 3) out vec3 worldPos;
layout(location = 4) out vec3 viewPos;

void main ()
{
	
	vec4 posVF = ubo.view * modelMatrix * vec4(aPos, 1);

	Normal = normalize(aNorm);
	
	worldPos = vec3(modelMatrix * vec4(aPos,1.0));

	viewPos = vec3(ubo.camPosition);

	gl_Position = ubo.proj * posVF; 
	
	fragTexCoord = aUv;
}
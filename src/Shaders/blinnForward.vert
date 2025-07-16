#version 450 
#extension GL_KHR_vulkan_glsl : enable


layout(std140, binding = 0) uniform uTransformObject {
    mat4 view;
    mat4 proj;
} ubo;

layout (push_constant) uniform pc
{
	mat4 modelMatrix;
};


layout( location = 0 ) in vec3 aPos;
layout( location = 1 ) in vec3 aNorm;
layout( location = 2 ) in vec2 aUv;




layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 Normal;
layout(location = 3) out vec3 worldPos;
layout(location = 4) out vec3 viewPos;

void main ()
{
	
	gl_Position = ubo.proj * ubo.view * modelMatrix * vec4(aPos, 1);
	worldPos = vec3(modelMatrix * vec4(aPos, 1.0));

	Normal = mat3(transpose(inverse(modelMatrix))) * aNorm;

	viewPos = vec3(0.f);
	
	fragTexCoord = aUv;
}
#version 450 
#extension GL_KHR_vulkan_glsl : enable


layout(std140, binding = 0) uniform uTransformObject {
    mat4 view;
    mat4 proj;
	vec3 camPosition;
} ubo;

layout(std140, binding = 2) uniform uLight 
{
    vec3 pos;
	vec3 albedo;
	vec3 ambient;
	vec3 specular;
	vec3 shininess; //for data alignment, made a float vec3
} light;

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
layout(location = 5) out vec3 lightPos;

void main ()
{
	
	gl_Position = ubo.proj * ubo.view * modelMatrix * vec4(aPos, 1);
	worldPos = vec3(modelMatrix * vec4(aPos, 1.0));

	Normal = mat3(transpose(inverse(modelMatrix))) * aNorm;
	
	lightPos = light.pos;

	viewPos = vec3(0.f);
	
	fragTexCoord = aUv;
}
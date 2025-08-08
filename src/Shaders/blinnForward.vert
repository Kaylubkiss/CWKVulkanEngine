#version 450 

layout(std140, binding = 0) uniform uTransformObject {
    mat4 view;
    mat4 proj;

	vec3 camPos;

	vec3 lightPos;
	vec3 lightAmbient;
	vec3 lightAlbedo;
	vec3 lightSpecular;
	vec3 lightShininess; //for data alignment, made a float vec3
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

void main ()
{
	
	gl_Position = ubo.proj * ubo.view * modelMatrix * vec4(aPos, 1);
	worldPos = vec3(modelMatrix * vec4(aPos, 1.0));

	Normal = mat3(transpose(inverse(modelMatrix))) * aNorm;
	
	fragTexCoord = aUv;
}
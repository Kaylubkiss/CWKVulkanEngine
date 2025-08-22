#version 450

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aUV;

layout(binding = 0) uniform UBO {
	//standard transformations.
	mat4 view;
	mat4 proj;

	//lightWorld
	mat4 depthVP;
	
	//camera
	vec3 camPos;

	vec3 lightPos; /* position of light */
	vec3 lightAmbient; /* scene color */
	vec3 lightAlbedo; /* base color of light */
	vec3 lightSpecular; /* reflectivity of the light */
	vec3 lightShininess; /* exponent value */


} ubo;

layout(push_constant) uniform pc
{
	mat4 modelMatrix;
};


layout(location = 0) out vec3 worldNormal;
layout(location = 1) out vec3 lightDir;
layout(location = 2) out vec3 viewDir;
layout(location = 3) out vec4 shadowCoord;

const mat4 biasMat = mat4(
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0, //don't do anything to the z-coord because we use depth bias
	0.5, 0.5, 0.0, 1.0
);

void main()
{
	gl_Position = ubo.proj * ubo.view * modelMatrix * vec4(aPos, 1);
	
	vec3 worldPos = vec3(modelMatrix * vec4(aPos, 1));

	viewDir = vec3(ubo.camPos - worldPos);
	lightDir = ubo.lightPos - worldPos;
	worldNormal = vec3(inverse(transpose(modelMatrix)) * vec4(aNormal,0));
	
	shadowCoord = (biasMat * ubo.depthVP * modelMatrix) * vec4(aPos, 1); //need to transform all the projected coords in NDC space to UV space.
}
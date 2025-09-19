#version 450


layout(std140, binding = 0) uniform uTransformObject
{
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
layout( location = 3 ) in vec3 aColor; //instead of texture coords, can also define color

layout( location = 0 ) out vec4 outWorldNormal;
layout( location = 1 ) out vec4 outWorldPosition;
layout( location = 2 ) out vec2 outTexCoord;
layout( location = 3 ) out vec4 outColor;

void main()
{
	gl_Position = ubo.proj * ubo.view * modelMatrix * vec4(aPos, 1);
	
	outWorldNormal = normalize(transpose(inverse(modelMatrix)) * vec4(aNorm, 0));
	outWorldPosition = modelMatrix * vec4(aPos, 1);
	outTexCoord = aUv;
	outColor = vec4(aColor, 1);
}
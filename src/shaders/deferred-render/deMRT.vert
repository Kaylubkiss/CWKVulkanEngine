#version 450

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
layout( location = 3 ) in vec3 aTan;
layout( location = 4 ) in vec3 aBitan;

layout( location = 0 ) out vec4 outWorldPosition;
layout( location = 1 ) out vec4 outWorldNormal;
layout( location = 2 ) out vec2 outTexCoord;
layout( location = 3 ) out mat3 outTBN;

void main()
{
	gl_Position = camera.proj * camera.view * modelMatrix * vec4(aPos, 1);
	
	outWorldPosition = modelMatrix * vec4(aPos, 1);

    mat4 normalMatrix = transpose(inverse(modelMatrix));

    vec3 T = normalize(vec3(normalMatrix * vec4(aTan, 0.0)));
    vec3 B = normalize(vec3(normalMatrix * vec4(aBitan, 0.0)));
    vec3 N = normalize(vec3(normalMatrix * vec4(aNorm, 0.0)));

    outTBN = mat3(T, B, N);

	outWorldNormal = vec4(N, 0.0);

	outTexCoord = aUv;
}
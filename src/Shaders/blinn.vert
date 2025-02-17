#version 450 
#extension GL_KHR_vulkan_glsl : enable


layout(binding = 0) uniform uTransformObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} uTransform;


layout( location = 0 ) in vec3 aPos;
layout( location = 1 ) in vec3 aNorm;
layout( location = 2 ) in vec2 aUv;


layout (push_constant) uniform Matrix
{
	mat4 modelMatrix;
} matrix;

layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 viewDir;
layout(location = 3) out vec3 normal;

//centered around unit square --> make sure to orientate ccw
vec2 positions[3] = vec2[3]
(
	 vec2(0.0, -0.5),
     vec2(-0.5, 0.5),
     vec2(0.5, 0.5)
);
//r,g,b
vec3 colors[3] = vec3[3]
(
	vec3(1.0, 0.0, 0.0),
	vec3(0.0, 1.0, 0.0),
	vec3(0.0, 0.0, 1.0)
);



void main ()
{

	vec4 posVF = uTransform.view * matrix.modelMatrix * vec4(aPos, 1);

	/*this is to transform normals and 
	prevent scaling from ruining the orthogonality of the normal*/

	//very expensive 
	mat4 nMV = transpose(inverse(uTransform.view * matrix.modelMatrix));
	
	normal = vec3(nMV * vec4(aNorm,0));

	gl_Position = uTransform.proj * posVF; 

	viewDir = -posVF.xyz;

	fragTexCoord =  aUv;
}

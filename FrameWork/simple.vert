#version 450 
#extension GL_KHR_vulkan_glsl : enable


layout(set = 0, binding = 0) uniform uTransformObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} uTransform;

layout( location = 0 ) in vec3 aPos;
layout( location = 1 ) in vec3 aNorm;
layout( location = 2 ) in vec2 aUv;
layout( location = 3 ) uniform mat4 aModel;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

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
	
	gl_Position = uTransform.proj * uTransform.view * uTransform.model * vec4(aPos, 1.f); 
	fragColor = aNorm;
	fragTexCoord = aUv;
}

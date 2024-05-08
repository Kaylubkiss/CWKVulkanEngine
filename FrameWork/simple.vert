#version 450 
#extension GL_KHR_vulkan_glsl : enable


layout(set = 0, binding = 0) uniform uTransformObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} uTransform;

layout(location = 0) out vec3 fragColor;

//centered around unit square --> make sure to orientate ccw
vec2 positions[3] = vec2[3]
(
	 vec2(0.0, -0.5),
     vec2(0.5, 0.5),
     vec2(-0.5, 0.5)
);
//r,g,b
vec3 colors[3] = vec3[3]
(
	vec3(1.0, 0.0, 0.0),
	vec3(0.0, 1.0, 0.0),
	vec3(0.0, 0.0, 1.0)
);


//do nothing shader
void main ()
{
	
	gl_Position =  vec4(positions[gl_VertexIndex], 0.f, 1.f); //gl_VertexID for OpenGL --> it's a vulkan extension idea.
	fragColor = colors[gl_VertexIndex];
}

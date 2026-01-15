#version 450

#define LIGHT_COUNT 2

layout(triangles, invocations = LIGHT_COUNT) in;
layout(triangle_strip, max_vertices = 3) out;

layout (std140, set = 0, binding = 0) uniform lightUBO 
{
	mat4 depthVP[LIGHT_COUNT]; 
} ubo;

void main()
{
	//for each 
	gl_Layer = gl_InvocationID;
	for (int i = 0; i < gl_in.length(); ++i) //gl_in.length() returns 3 in this case as we are to process triangles
	{	
		gl_Position = ubo.depthVP[gl_Layer] * gl_in[i].gl_Position;
		EmitVertex();
	}
	EndPrimitive();
}

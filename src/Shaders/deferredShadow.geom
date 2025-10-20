#version 450

#define LIGHT_COUNT 2

layout(triangles, invocations = LIGHT_COUNT) in;
layout(triangle_strip, max_vertices = 3) out;

layout (binding = 0) uniform UBO 
{
	mat4 depthVP[LIGHT_COUNT]; 
} lightUBO;

layout(location = 0) in vec4 inWorldPos[];

void main()
{
	//for each 
	for (int i = 0; i < gl_in.length(); ++i) //gl_in.length() returns 3 in this case as we are to process triangles
	{
		gl_Layer = gl_InvocationID;
		gl_Position = lightUBO.depthVP[gl_InvocationID] * (inWorldPos[i]);
		EmitVertex();
	}
	EndPrimitive();
}

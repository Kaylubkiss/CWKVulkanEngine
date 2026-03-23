#version 450

#define LIGHT_COUNT 2

layout(location = 0) in vec3 aPos;

layout (push_constant) uniform pc
{
	mat4 modelMatrix;
};

void main()
{

	gl_Position = modelMatrix * vec4(aPos,1);
}
#version 450

layout(location = 0) in vec3 aPos;

layout (push_constant) uniform pc
{
	mat4 modelMatrix;
};

layout(location = 0) out vec4 outWorldPos;

void main()
{

	outWorldPos = modelMatrix * vec4(aPos,1);
	//wait to assign gl_Position in geometry shader
}
#version 450

//not necessary to make this file.
layout(location = 0) in vec2 fragTexCoord;

layout(binding = 1) uniform sampler2D shadowMap;

layout(location = 0) out vec4 color;


void main(){
		float depthValue = texture(shadowMap, fragTexCoord).r;
		color = vec4(vec3(depthValue), 1);
}
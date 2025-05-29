#version 450 

layout(location = 1) in vec2 fragTexCoord;
layout(binding = 1) uniform sampler2D texSampler;


layout(binding = 2) uniform uLight 
{
	vec3 albedo;
} light;

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = texture(texSampler, fragTexCoord);
	outColor.rgb *= light.albedo;
	outColor.a = 1;
}														
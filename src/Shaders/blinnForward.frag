#version 450 

layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 worldPos;

layout(std140, binding = 0) uniform uTransformObject {
    mat4 view;
    mat4 proj;

	vec3 camPos;

	float lightShininess;
	vec3 lightPos;
	vec3 lightAmbient;
	vec3 lightAlbedo;
	vec3 lightSpecular;
} ubo;

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main()
{

	vec3 norm = normalize(Normal);
	vec3 lightDir = normalize(ubo.lightPos - worldPos);
	
	vec3 viewDir = normalize(ubo.camPos - worldPos);
	vec3 halfwayDir = normalize(lightDir + viewDir);

	float spec = pow(max(dot(norm, halfwayDir), 0.0), ubo.lightShininess.x);
	vec3 specular = spec * ubo.lightAlbedo;  
	
	float diff = max(dot(norm, lightDir), 0.0f);
	vec3 diffuse = diff * ubo.lightAlbedo;
	
	//outColor = texture(texSampler, fragTexCoord);
	//outColor.rgb = (ubo.lightAmbient + diffuse + specular) * outColor.rgb;
	outColor.rgb = ubo.lightAmbient + diffuse + specular;
	outColor.a = 1;
}														
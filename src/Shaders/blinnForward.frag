#version 450 

layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 worldPos;
layout(location = 4) in vec3 viewPos;

layout(binding = 1) uniform sampler2D texSampler;


layout(binding = 2) uniform uLight 
{
    vec3 pos;
	vec3 albedo;
	vec3 ambient;
	vec3 specular;
	float shininess;
} light;

layout(location = 0) out vec4 outColor;

void main()
{

	vec3 norm = normalize(Normal);
	vec3 lightDir = normalize(light.pos - worldPos);
	float diff = max(dot(norm, lightDir), 0.0f);

	vec3 diffuse = diff * light.albedo;

	vec3 viewDir = normalize(viewPos - worldPos);
	vec3 reflectDir = reflect(-lightDir, norm); 

	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.f);
	vec3 specular = 0.5 * spec * light.albedo;  
	
	outColor = texture(texSampler, fragTexCoord);

	outColor.rgb = (light.ambient + diffuse + specular) * outColor.rgb;
	outColor.a = 1;
}														
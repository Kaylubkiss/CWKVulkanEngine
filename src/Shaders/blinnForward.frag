#version 450 

layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 worldPos;
layout(location = 4) in vec3 viewPos;


layout(std140, binding = 2) uniform uLight 
{
    vec3 pos;
	vec3 albedo;
	vec3 ambient;
	vec3 specular;
	vec3 shininess; //for data alignment, made a float vec3
} light;

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main()
{

	vec3 norm = normalize(Normal);
	vec3 lightDir = normalize(light.pos - worldPos);
	
	vec3 viewDir = normalize(viewPos - worldPos);
	vec3 halfwayDir = normalize(lightDir + viewDir);

	float spec = pow(max(dot(norm, halfwayDir), 0.0), light.shininess.x);
	vec3 specular = spec * light.albedo;  
	
	float diff = max(dot(norm, lightDir), 0.0f);
	vec3 diffuse = diff * light.albedo;
	
	outColor = texture(texSampler, fragTexCoord);
	outColor.rgb = (light.ambient + diffuse + specular) * outColor.rgb;
	outColor.a = 1;
}														
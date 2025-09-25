#version 450


layout(binding = 1) uniform sampler2D samplerPosition;
layout(binding = 2) uniform sampler2D samplerNormal;
layout(binding = 3) uniform sampler2D samplerAlbedo;

layout(location = 0) in vec2 inUV;



layout(binding = 4) uniform lightUBO
{
	
	float shininess; /* exponent value */
	vec3 position; /* position of light */
	vec3 ambient; /* scene color */
	vec3 albedo; /* base color of light */
	vec3 specular; /* reflectivity of the light */

	vec3 viewPosition; /* position of the camera (for view direction calculation) */

} light;

layout(location = 0) out vec4 fragColor;

void main()
{
	vec3 albedo = texture(samplerAlbedo, inUV).rgb;
	vec3 normal = texture(samplerNormal, inUV).rgb;
	vec3 position = texture(samplerPosition, inUV).rgb;

	vec3 lightDir = normalize(light.position - position);
	
	vec3 viewDir = normalize(light.viewPosition-position);
	vec3 halfwayDir = normalize(lightDir + viewDir);

	float spec = pow(max(dot(normal, halfwayDir), 0.0), light.shininess);
	vec3 specular = spec * albedo;  
	
	float diff = max(dot(normal, lightDir), 0.0f);
	vec3 diffuse = diff * albedo;

	fragColor.rgb = light.ambient + diffuse + specular; 
	//fragColor.rgb = position; 
	fragColor.a = 1.f;
}
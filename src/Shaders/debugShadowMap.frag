#version 450




layout (binding = 1) uniform sampler2D samplerColor;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

layout (binding = 0) uniform UBO 
{
	//standard transformations.
	mat4 view;
	mat4 proj;

	//lightWorld
	mat4 depthVP;
	
	//camera
	vec3 camPos;

	vec3 lightPos; /* position of light */
	vec3 lightAmbient; /* scene color */
	vec3 lightAlbedo; /* base color of light */
	vec3 lightSpecular; /* reflectivity of the light */
	vec3 lightShininess; /* exponent value */
} ubo;

float LinearizeDepth(float depth)
{
  float n = 1.0f;
  float f = 96.f;
  float z = depth;
  return (2.0 * n) / (f + n - z * (f - n));	
}

void main() 
{
	float depth = texture(samplerColor, inUV).r;
	outFragColor = vec4(vec3(1-LinearizeDepth(depth)), 1.0);
}
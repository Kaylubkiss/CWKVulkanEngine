#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 0) uniform sampler2D samplerPosition;
layout(set = 0, binding = 1) uniform sampler2D samplerNormal;
layout(set = 0, binding = 2) uniform sampler2D samplerAlbedo;
layout(set = 0, binding = 3) uniform sampler2DArray samplerShadowMap;

#define LIGHT_COUNT 2
#define AMBIENT_COLOR .2
#define M_PI 4.0 * atan(1.0)
#define MAT_ROUGHNESS 0.3f
#define MAT_METALLIC 0.1f

struct Light 
{
	float shininess; /* exponent value */
	vec3 position; /* position of light */
	vec3 ambient; /* scene color */
	vec3 albedo; /* base color of light */
	vec3 specular; /* reflectivity of the light */
	mat4 viewMatrix; /* projects a point to the light's POV */
};

layout(set = 1, binding = 0) uniform UBO
{
	Light lights[LIGHT_COUNT];
	vec3 viewPosition; /* position of the camera (for view direction calculation) */
} ubo;



float ShadowSampling(vec4 fragPos)
{
	float shadow = 1.0;

	for (int layer = 0; layer < LIGHT_COUNT; ++layer)
	{
		vec4 shadowClip = ubo.lights[layer].viewMatrix * fragPos;
		vec3 shadowCoord = shadowClip.xyz / shadowClip.w;

		shadowCoord.xy = shadowCoord.xy * 0.5 + 0.5;

		if (shadowCoord.z >= 0.0 && shadowCoord.z <= 1.0)
		{
		    float dist = texture(samplerShadowMap, vec3(shadowCoord.xy, layer)).r;

		    if (dist < shadowCoord.z)
			{
				shadow *= AMBIENT_COLOR;
			}
		}
	}

    return shadow;
}


vec3 Radiance(vec3 P, vec3 L, vec3 lightAlbedo)
{
    //because it's a point light, we will just attenuate the intensity of the light's color.
    float distance = length(L - P);
    float attenuation = 1.0 / (distance * distance + 0.0001); //just in-case the distance is 0, add a small fraction.
    //NOTE: might want to use a quadratic version of attenuation for more control of the roll-off.

    return (lightAlbedo * attenuation);
}

float GeometrySchlickGGX(float NdotX)
{

    //k is computed based on direct lighting (which is the lighting method I'm using for now)
    float kNum = MAT_ROUGHNESS + 1.0;
    float kDirect = (kNum * kNum) / 8.0;

    return ((NdotX) / (NdotX * (1-kDirect) + kDirect));
}

float GeometrySmith(float NdotV, float NdotL)
{

    float ggx1 = GeometrySchlickGGX(NdotV);
    float ggx2 = GeometrySchlickGGX(NdotL);

    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    //clamp to avoid black spots
    return (F0 + (1.0-F0) * pow(clamp(1.0-cosTheta, 0.0, 1.0), 5.0));
}


float DistributionGGX(float NdotH)
{
    float a = MAT_ROUGHNESS * MAT_ROUGHNESS;
    float a2 = a * a;

    float denom = NdotH * NdotH * (a2 - 1) + 1;
    denom = M_PI * denom * denom;

    return (a2 / denom);
}

vec3 Fr(vec3 P, vec3 L,
    vec3 N, vec3 H, vec3 V)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float NdotH = max(dot(N, H), 0.0);

    vec3 albedo = texture(samplerAlbedo, inUV).rgb;

    float NDF = DistributionGGX(NdotH);
    float G   = GeometrySmith(NdotV, NdotL);

    float baseReflectivity = 0.04f;
    vec3 F0                = mix(vec3(baseReflectivity), albedo, MAT_METALLIC);

    vec3 F   = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 nom    =  NDF * F * G;
    float denom = 4.0 * NdotV * NdotL; //want to make sure division by 0 is impossible

    vec3 specular = nom / max(denom, 0.0001);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - MAT_METALLIC; //because metals don't refract, we nullify the diffuse portion if it's 100% metal.

    return ((kD * albedo / M_PI) + specular);
}


vec3 CookTorrenceReflectance(vec3 P, vec3 N)
{
    vec3 Lo = vec3(0.f);
    vec3 V = normalize(ubo.viewPosition - P);

    for (int i = 0; i < LIGHT_COUNT; ++i)
    {
        vec3 L = normalize(ubo.lights[i].position - P);
        vec3 H = normalize(L + V);

        Lo += Fr(P, L, N, H, V) * Radiance(P, ubo.lights[i].position, ubo.lights[i].albedo) *  max(dot(N, L), 0.0);
    }

    return Lo;
}


void main()
{
	vec3 position = texture(samplerPosition, inUV).rgb;
	vec3 normal = texture(samplerNormal, inUV).rgb;
    vec3 albedo = texture(samplerAlbedo, inUV).rgb;

    //NOTE:
    // NdotL computed multiple times
    // F0 computed multiple times
    // albedo computed multiple times

    fragColor.rgb = AMBIENT_COLOR * albedo + CookTorrenceReflectance(position, normal);

    //gamma correction using the Reinhard operator -- I DON'T REALLY UNDERSTAND THIS
    fragColor.rgb = fragColor.rgb / (fragColor.rgb + vec3(1.0));
    fragColor.rgb = pow(fragColor.rgb, vec3(1.0/2.2));

	//fragColor.rgb *= vec3(ShadowSampling(vec4(position, 1)));

	fragColor.a = 1.f;
}
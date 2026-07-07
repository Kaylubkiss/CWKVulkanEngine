#version 450 core

#define LIGHT_COUNT 2
#define AMBIENT_FACTOR .5
#define M_PI 4.0 * atan(1.0)

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 fragColor;

struct Light
{
	vec3 position; /* position of light */
	vec3 albedo; /* base color of light */
	mat4 viewMatrix; /* projects a point to the light's POV */
};

layout(set = 0, binding = 0) uniform UBO
{
	Light lights[LIGHT_COUNT];
	vec3 viewPosition; /* position of the camera (for view direction calculation) */
} ubo;

//holy ffreck
layout(set = 1, binding = 0) uniform sampler2D samplerPosition;
layout(set = 1, binding = 1) uniform sampler2D samplerNormal;
layout(set = 1, binding = 2) uniform sampler2D samplerAlbedo;
layout(set = 1, binding = 3) uniform sampler2D samplerMetallicRoughness;
layout(set = 1, binding = 4) uniform sampler2D samplerAmbientOcclusion;
layout(set = 1, binding = 5) uniform sampler2DArray samplerShadowMap;
layout(set = 1, binding = 6) uniform samplerCube irradianceMap;
layout(set = 1, binding = 7) uniform samplerCube prefilterMap;
layout(set = 1, binding = 8) uniform sampler2D brdfLUT;


float ShadowSampling(vec4 fragPos, float NdotL, int i)
{
	float shadow = 1.0;

    vec4 shadowClip = ubo.lights[i].viewMatrix * fragPos;
    vec3 shadowCoord = shadowClip.xyz / shadowClip.w;

    shadowCoord.xy = shadowCoord.xy * 0.5 + 0.5;

    if (shadowCoord.z < 1.0)
    {
        float currentDepth = shadowCoord.z;

        float closestDepth = texture(samplerShadowMap, vec3(shadowCoord.xy, i)).r;

        //bias here would cause problems due to the depth bias already applied.
        //float bias = max(0.05 * (1.0 - NdotL), .005);

        if (currentDepth > closestDepth)
        {
            shadow = AMBIENT_FACTOR;
        }
    }

    return shadow;
}


vec3 Radiance(vec3 fragPos, vec3 lightPos, vec3 lightAlbedo)
{
    //because it's a point light, we will just attenuate the intensity of the light's color.

    float dist = length(lightPos - fragPos);
    float attenuation = 1.0 / (dist * dist + 0.0001); //just in-case the distance is 0, add a small fraction.

    //NOTE: might want to use a quadratic version of attenuation for more control of the roll-off.

    return (lightAlbedo * attenuation);
}

float GeometrySchlickGGX(float NdotX, float roughness)
{

    //k is computed based on direct lighting (which is the lighting method I'm using for now)
    float kNum = roughness + 1.0;
    float kDirect = (kNum * kNum) / 8.0;

    return ((NdotX) / (NdotX * (1.0-kDirect) + kDirect));
}

float GeometrySmith(float NdotV, float NdotL, float roughness)
{
    return GeometrySchlickGGX(NdotV, roughness) *
           GeometrySchlickGGX(NdotL, roughness);
}

vec3 FresnelSchlick(float cosTheta, vec3 F0, float roughness)
{
    //clamp to avoid black spots
    return F0 + (max(vec3(1.0-roughness), F0) - F0) * pow(clamp(1.0-cosTheta, 0.0, 1.0), 5.0);
}


float DistributionGGX(float NdotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;

    float denom = NdotH * NdotH * (a2 - 1.0) + 1;
    denom = M_PI * denom * denom;

    return (a2 / denom);
}

void main()
{
	vec3 P = texture(samplerPosition, inUV).rgb;

	//linear filtering during the texture sampling process still distorts normals
	vec3 N = normalize(texture(samplerNormal, inUV).rgb);
	vec3 V = normalize(ubo.viewPosition - P);
	vec3 R = reflect(-V, N);

    vec3 albedo = texture(samplerAlbedo, inUV).rgb;

    float ao = texture(samplerAmbientOcclusion, inUV).r;

    //g = roughness, b = metalness
    vec3 metallicRoughness = texture(samplerMetallicRoughness, inUV).rgb;

    float metalness = metallicRoughness.b;
    float roughness = metallicRoughness.g;

    if (ao == 0.0)
    {
        ao = 1.0;
    }

    if (roughness == 0.0)
    {
        roughness = 1.0;
    }

    float NdotV = max(dot(N, V), 0.0);

    float baseReflectivity = 0.04f;
    vec3 F0 = mix(vec3(baseReflectivity), albedo, metalness);

    vec3 Lo = vec3(0.f);
    for (int i = 0; i < LIGHT_COUNT; ++i)
    {
        vec3 L = normalize(ubo.lights[i].position - P);
        vec3 H = normalize(L + V);

        float NdotL = max(dot(N, L), 0.0);
        float NdotH = max(dot(N, H), 0.0);

        float NDF = DistributionGGX(NdotH, roughness);
        float G   = GeometrySmith(NdotV, NdotL, roughness);

        vec3 F  = FresnelSchlick(NdotV, F0, roughness);

        vec3 nom    =  NDF * F * G;
        float denom = 4.0 * NdotV * NdotL + .0001; //want to make sure division by 0 is impossible

        vec3 specular = nom / denom;

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;

        kD *= 1.0 - metalness; //because metals don't refract, we nullify the diffuse portion if it's 100% metal.

        float shadowFactor = ShadowSampling(vec4(P, 1), NdotL, i);
        vec3 radiance = Radiance(P, ubo.lights[i].position, ubo.lights[i].albedo);

        Lo += (kD * albedo / M_PI + specular) * radiance * NdotL;
    }

    vec3 F = FresnelSchlick(NdotV, F0, roughness);

    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metalness;

    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse = irradiance * albedo;



    const float MAX_REFLECTION_LOD = log2(512);
    vec3 prefilteredColor = textureLod(prefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
    vec2 envBRDF = texture(brdfLUT, vec2(NdotV, roughness)).rg;
    vec3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);

    vec3 ambientColor = (kD * diffuse + specular) * ao; //diffuse IBL

    fragColor.rgb = ambientColor + Lo;
	fragColor.a = 1.0;
}
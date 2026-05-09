#version 450 core
#extension GL_KHR_vulkan_glsl : enable

layout(set = 1, binding = 0) uniform sampler2D samplerSceneColor;

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 fragColor;

//code courtesy of: https://64.github.io/tonemapping/#uncharted-2
vec3 uncharted2_tonemap_partial(vec3 x)
{
    float A = 0.15f;
    float B = 0.50f;
    float C = 0.10f;
    float D = 0.20f;
    float E = 0.02f;
    float F = 0.30f;
    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

vec3 uncharted2_filmic(vec3 v)
{
    float exposure_bias = 10.f;
    vec3 curr = uncharted2_tonemap_partial(v * exposure_bias);

    vec3 W = vec3(11.2f);
    vec3 white_scale = vec3(1.0f) / uncharted2_tonemap_partial(W);
    return curr * white_scale;
}

void main()
{
    fragColor = texture(samplerSceneColor, inUV);

    fragColor.rgb = uncharted2_filmic(fragColor.rgb);

   float gamma = 2.2;
   fragColor.rgb = pow(fragColor.rgb, vec3(1.0/gamma));

    fragColor.a = 1.f;
}
#version 450 core

//TODO: make set = 2 an included global variable like MATERIAL. Will need include support/slang
layout(set = 2, binding = 0) uniform sampler2D equirectangularMap;

layout(location = 0) in vec3 inUVW;
layout(location = 0) out vec4 fragColor;

vec2 invAtan = vec2(0.1591, 0.3183); // vec2(1/2PI, 1/PI);


vec2 SampleSphericalMap(vec3 localPos)
{
    vec2 uv;

    float theta = atan(localPos.z, localPos.x);
    float phi = asin(-localPos.y);

    uv.x = theta;
    uv.y = phi;

    uv *= invAtan;
    uv += 0.5;

    return uv;
}

void main()
{
      vec2 uv = SampleSphericalMap(normalize(inUVW));
      vec3 color = texture(equirectangularMap, uv).rgb;

      fragColor = vec4(color, 1.0);
}
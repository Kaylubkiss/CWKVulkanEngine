#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(set = 0, binding = 0) uniform sceneUBO
{
	mat4 view;
	mat4 proj;
} camera;

layout( location = 0 ) out vec3 outUVW;

void main()
{
    vec3 curr_corner = vec3((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2, 1);
    curr_corner.xy = curr_corner.xy * 2.0f - 1.0f;

    gl_Position = vec4(curr_corner, curr_corner.z);

    //must take out the translation component so that the sky doesn't translate with objects.
    mat4 invView = transpose(mat4(mat3(camera.view)));
    mat4 invProj = inverse(camera.proj);

    outUVW = vec3(invView * invProj * vec4(curr_corner, 1));

}
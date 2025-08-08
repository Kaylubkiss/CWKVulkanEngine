#include "vkMesh.h"
#include <glm/glm.hpp>
#define epsilon .0001
#include "ApplicationGlobal.h"

using namespace glm;
#define PI atan(1.0) * 4.0
#define HALF_PI PI * 0.5
#define YINDEX 1
#define XINDEX 0

bool DegenerateTri(const vec3& p0, const vec3& p1, const vec3& p2) 
{
    return (0.5 * glm::length(glm::cross(p1 - p0, p2 - p0)) <= epsilon);
}

void BuildIndexBuffer(int stacks, int slices, Mesh& mesh)
{
    //@todo: IMPLEMENT ME
    int i0 = 0, i1 = 0, i2 = 0;

    int stride = slices + 1;

    for (int i = 0; i < stacks; ++i)
    {
        int curr_row = i * stride;

        for (int j = 0; j < slices; ++j)
        {

            /*  You need to compute the indices for the first triangle here */
            /*  ... */
            i0 = curr_row + j; //p0
            i1 = i0 + 1; //p1
            i2 = i1 + stride; //p2

            /*  Ignore degenerate triangle */
            if (!DegenerateTri(mesh.data.vertices[i0].pos,
                mesh.data.vertices[i1].pos,
                mesh.data.vertices[i2].pos))
            {
                /*  Add the indices for the first triangle */
                mesh.data.indices.push_back(i0);
                mesh.data.indices.push_back(i1);
                mesh.data.indices.push_back(i2);
            }


            /*  You need to compute the indices for the second triangle here */
            /*  ... */
            int p0_fft = i0;

            i0 = i2; //p3 = p2
            i1 = i0 - 1; //p4 = p3 - 1
            i2 = p0_fft; //p5 = p0

            /*  Ignore degenerate triangle */
            if (!DegenerateTri(mesh.data.vertices[i0].pos,
                mesh.data.vertices[i1].pos,
                mesh.data.vertices[i2].pos))
            {
                /*  Add the indices for the second triangle */
                mesh.data.indices.push_back(i0);
                mesh.data.indices.push_back(i1);
                mesh.data.indices.push_back(i2);
            }
        }
    }
}

Mesh Mesh::GeneratePlane(int stacks, int slices) 
{
    Mesh mesh;

    for (int stack = 0; stack <= stacks; ++stack)
    {
        float row = (float)stack / stacks;

        for (int slice = 0; slice <= slices; ++slice)
        {
            float col = (float)slice / slices;

            Vertex v;

            v.pos = glm::vec3(col - 0.5f, row - 0.5f, 0.0f);
            v.nrm = glm::vec3(0.0f, 0.0f, 1.0f);
            v.uv = glm::vec2(col, row);

            mesh.data.vertices.push_back(v);
        }
    }

    BuildIndexBuffer(stacks, slices, mesh);

    return mesh;

}

Mesh Mesh::GenerateCube(int stacks, int slices) 
{
    Mesh planeMesh = GeneratePlane(stacks, slices);
    Mesh mesh;

    int offset = 0;
    vec3 const translateArray[] =
    {
        vec3(+0.0f, +0.0f, +0.5f), // Z+
        vec3(+0.0f, +0.0f, -0.5f), // Z-
        vec3(+0.5f, +0.0f, +0.0f), // X+
        vec3(-0.5f, +0.0f, +0.0f), // X-
        vec3(+0.0f, +0.5f, +0.0f), // Y+
        vec3(+0.0f, -0.5f, +0.0f), // Y-
    };

    vec2 const rotateArray[] =
    {
        vec2(+0.0f, +0.0f),             // Z+
        vec2(+0.0f, (float)+PI),        // Z-
        vec2(+0.0f, (float)+HALF_PI),   // X+       
        vec2(+0.0f, (float)-HALF_PI),   // X-
        vec2((float)-HALF_PI, +0.0f),   // Y+
        vec2((float)+HALF_PI, +0.0f)    // Y-
    };


    /*  Transform the plane to 6 positions to form the faces of the cube */
    for (int i = 0; i < 6; ++i)
    {
        mat4 transformMat = translate(mat4(1.0), translateArray[i]) *
            rotate(glm::mat4(1.0), rotateArray[i][YINDEX], vec3(0,1,0)) *
            rotate(glm::mat4(1.0), rotateArray[i][XINDEX], vec3(1,0,0));

        for (int j = 0; j < planeMesh.data.vertices.size(); ++j)
        {
            Vertex v;
            v.pos = vec3(transformMat * vec4(planeMesh.data.vertices[j].pos, 1.0));
            v.nrm = vec3(transformMat * vec4(planeMesh.data.vertices[j].nrm, 1.0));
            v.uv = planeMesh.data.vertices[j].uv;

          /*  v.pos = RoundDecimal(v.pos);
            v.nrm = RoundDecimal(v.nrm);*/

            mesh.data.vertices.push_back(v);
        }

        for (int j = 0; j < planeMesh.data.indices.size(); ++j) 
        {
            mesh.data.indices.push_back(planeMesh.data.indices[j] + offset);
        }

        offset += planeMesh.data.indices.size();
    }

    return mesh;

}
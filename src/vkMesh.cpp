#include "vkMesh.h"
#include <glm/glm.hpp>
#include "ApplicationGlobal.h"
#include <algorithm>
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#define PI atan(1.0) * 4.0
#define HALF_PI PI * 0.5
#define YINDEX 1
#define XINDEX 0
#define EPSILON .0001f

bool DegenerateTri(const glm::vec3 & p0, const glm::vec3& p1, const glm::vec3& p2)
{
    return (0.5 * glm::length(glm::cross(p1 - p0, p2 - p0)) <= EPSILON);
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
    glm::vec3 const translateArray[] =
    {
        glm::vec3(+0.0f, +0.0f, +0.5f), // Z+
        glm::vec3(+0.0f, +0.0f, -0.5f), // Z-
        glm::vec3(+0.5f, +0.0f, +0.0f), // X+
        glm::vec3(-0.5f, +0.0f, +0.0f), // X-
        glm::vec3(+0.0f, +0.5f, +0.0f), // Y+
        glm::vec3(+0.0f, -0.5f, +0.0f), // Y-
    };

    glm::vec2 const rotateArray[] =
    {
        glm::vec2(+0.0f, +0.0f),             // Z+
        glm::vec2(+0.0f, (float)+PI),        // Z-
        glm::vec2(+0.0f, (float)+HALF_PI),   // X+       
        glm::vec2(+0.0f, (float)-HALF_PI),   // X-
        glm::vec2((float)-HALF_PI, +0.0f),   // Y+
        glm::vec2((float)+HALF_PI, +0.0f)    // Y-
    };


    /*  Transform the plane to 6 positions to form the faces of the cube */
    for (int i = 0; i < 6; ++i)
    {
        glm::mat4 transformMat = glm::translate(glm::mat4(1.0), translateArray[i]) *
            glm::rotate(glm::mat4(1.0), rotateArray[i][YINDEX], glm::vec3(0,1,0)) *
            glm::rotate(glm::mat4(1.0), rotateArray[i][XINDEX], glm::vec3(1,0,0));

        for (int j = 0; j < planeMesh.data.vertices.size(); ++j)
        {
            Vertex v;
            v.pos = glm::vec3(transformMat * glm::vec4(planeMesh.data.vertices[j].pos, 1.0));
            v.nrm = glm::vec3(transformMat * glm::vec4(planeMesh.data.vertices[j].nrm, 1.0));
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


void Mesh::ComputeVertexNormals() 
{
    std::vector<Vertex>& vertexBufferData = this->data.vertices;
    std::vector<uint16_t>& indexBufferData = this->data.indices;

    for (int i = 0; i < vertexBufferData.size(); ++i)
    {
        glm::vec3 total_vec(0.0f);

        for (int j = 0; j < indexBufferData.size(); ++j)
        {
            //total_vec
            if (indexBufferData[j] == i)
            {
                //angle = glm::angle(q -p, r - p)
                //total_vec += angle * cross(q - r, r - p)
                float angle;
                glm::vec3 normal;
                glm::vec3 orientation_QP;
                glm::vec3 orientation_RP;
                glm::vec3 edge_1;
                glm::vec3 edge_2;

                if (j % 3 == 0) //beginning of the face index
                {
                    orientation_QP = vertexBufferData[indexBufferData[j + 1]].pos - vertexBufferData[indexBufferData[j]].pos;
                    orientation_RP = vertexBufferData[indexBufferData[j + 2]].pos - vertexBufferData[indexBufferData[j]].pos;

                    edge_1 = orientation_QP;
                    edge_2 = orientation_RP;
                }
                else if (j % 3 == 1) //middle of the face index
                {
                    orientation_QP = vertexBufferData[indexBufferData[j]].pos - vertexBufferData[indexBufferData[j - 1]].pos;
                    orientation_RP = vertexBufferData[indexBufferData[j + 1]].pos - vertexBufferData[indexBufferData[j - 1]].pos;

                    edge_1 = vertexBufferData[indexBufferData[j - 1]].pos - vertexBufferData[indexBufferData[j]].pos;
                    edge_2 = vertexBufferData[indexBufferData[j + 1]].pos - vertexBufferData[indexBufferData[j]].pos;

                }
                else if (j % 3 == 2) //end of face index sequence
                {
                    orientation_QP = vertexBufferData[indexBufferData[j - 1]].pos - vertexBufferData[indexBufferData[j - 2]].pos;
                    orientation_RP = vertexBufferData[indexBufferData[j]].pos - vertexBufferData[indexBufferData[j - 2]].pos;

                    edge_1 = vertexBufferData[indexBufferData[j - 2]].pos - vertexBufferData[indexBufferData[j]].pos;
                    edge_2 = vertexBufferData[indexBufferData[j - 1]].pos - vertexBufferData[indexBufferData[j]].pos;
                }


                angle = glm::degrees(acos((abs(glm::dot(edge_1, edge_2)) /
                    (glm::length(edge_1) * glm::length(edge_2)))));
                normal = glm::cross(orientation_QP, orientation_RP);
                //The angle needs to be between the edges that *SHARE* the vertex.
                total_vec += (angle * normal);
            }
        }

        vertexBufferData[i].nrm = glm::normalize(total_vec); //point + vector equals another point
    }
}

void Mesh::ComputeVertices()
{
    VkPhysicalDevice p_device = _GraphicsContext->PhysicalDevice();
    VkDevice l_device = _GraphicsContext->LogicalDevice();

    int numVertices = static_cast<int>(this->data.vertices.size());

    glm::vec3 min_points(0.f);
    glm::vec3 max_points(0.f);

    for (unsigned i = 0; i < this->data.vertices.size(); ++i)
    {

        min_points.x = std::min(min_points.x, this->data.vertices[i].pos.x);
        min_points.y = std::min(min_points.y, this->data.vertices[i].pos.y);
        min_points.z = std::min(min_points.z, this->data.vertices[i].pos.z);

        max_points.x = std::max(max_points.x, this->data.vertices[i].pos.x);
        max_points.y = std::max(max_points.y, this->data.vertices[i].pos.y);
        max_points.z = std::max(max_points.z, this->data.vertices[i].pos.z);

        this->data.vertices[i].nrm = glm::vec3(0, 0, 0.f);

        this->center += this->data.vertices[i].pos;
    }

    this->center /= this->data.vertices.size();

    float unitScale = std::max({ glm::length(max_points.x - min_points.x), glm::length(max_points.y - min_points.y), glm::length(max_points.z - min_points.z) });

    max_points = { -std::numeric_limits<float>::min(),  -std::numeric_limits<float>::min() , -std::numeric_limits<float>::min() };
    min_points = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };

    for (size_t i = 0; i < this->data.vertices.size(); ++i)
    {
        this->data.vertices[i].pos = (this->data.vertices[i].pos - this->center) / unitScale;

        max_points.x = std::max(max_points.x, this->data.vertices[i].pos.x);
        max_points.y = std::max(max_points.y, this->data.vertices[i].pos.y);
        max_points.z = std::max(max_points.z, this->data.vertices[i].pos.z);

        min_points.x = std::min(min_points.x, this->data.vertices[i].pos.x);
        min_points.y = std::min(min_points.y, this->data.vertices[i].pos.y);
        min_points.z = std::min(min_points.z, this->data.vertices[i].pos.z);
    }

    this->maxLocalPoints = max_points;
    this->minLocalPoints = min_points;

    Mesh::ComputeVertexNormals();

    size_t sizeOfVertexBuffer = (sizeof(data.vertices[0]) * this->data.vertices.size());
    this->buffer.vertex = vk::Buffer(p_device, l_device, sizeOfVertexBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, this->data.vertices.data());
    
    size_t sizeOfIndexBuffer = (sizeof(data.indices[0]) * this->data.indices.size());
    this->buffer.index = vk::Buffer(p_device, l_device, sizeOfIndexBuffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, this->data.indices.data());
}

bool Mesh::LoadOBJMesh(const char* filePath) 
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filePath)) {
        std::cerr << err << std::endl;
        return false;
    }

    if (!warn.empty())
    {
        std::cout << warn << std::endl << std::endl;
    }

    std::unordered_map<Vertex, uint32_t> uniqueVertices = {};


    for (const auto& shape : shapes)
    {
        for (const auto& index : shape.mesh.indices)
        {
            Vertex vert = {};

            vert.pos =
            {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            if (index.texcoord_index > 0)
            {
                vert.uv =
                {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1 - attrib.texcoords[2 * index.texcoord_index + 1] //vulkan is upside down.
                };

            }

            if (uniqueVertices.count(vert) == 0)
            {
                uniqueVertices[vert] = static_cast<uint32_t>(data.vertices.size());
                data.vertices.push_back(vert);
            }

            data.indices.push_back(uniqueVertices[vert]);

        }
    }

    Mesh::ComputeVertices();

    std::cout << std::endl;
    std::cout << "Mesh loaded... " + std::string(filePath) << std::endl;

    return true;
}
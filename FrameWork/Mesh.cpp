#include "Mesh.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

Object::Object(const char* fileName, MeshType type)
{
    LoadMeshOBJ(fileName, this->mMesh);

    glm::vec3 min_points(0.f);
    glm::vec3 max_points(0.f);

    for (unsigned i = 0; i < this->mMesh.vertexBufferData.size(); ++i)
    {
        min_points.x = std::min(min_points.x, mMesh.vertexBufferData[i].pos.x);
        min_points.y = std::min(min_points.y, mMesh.vertexBufferData[i].pos.y);
        min_points.z = std::min(min_points.z, mMesh.vertexBufferData[i].pos.z);

        max_points.x = std::max(max_points.x, mMesh.vertexBufferData[i].pos.x);
        max_points.y = std::max(max_points.y, mMesh.vertexBufferData[i].pos.y);
        max_points.z = std::max(max_points.z, mMesh.vertexBufferData[i].pos.z);

        mMesh.vertexBufferData[i].nrm = glm::vec3(2.f, .5f, 0.f);
    }

    mCenter = (max_points + min_points) / 2.f;
    float unitScale = std::max({ glm::length(max_points.x - min_points.x), glm::length(max_points.y - min_points.y), glm::length(max_points.z - min_points.z) });

    for (size_t i = 0; i < this->mMesh.vertexBufferData.size(); ++i)
    {
        this->mMesh.vertexBufferData[i].pos = (this->mMesh.vertexBufferData[i].pos - mCenter) / unitScale;
    }

  
    //if (type == M_CUBE) 
    //{
    //    //back face
    //    mMesh.vertexBufferData[0].uv = { 1,1 };
    //    mMesh.vertexBufferData[2].uv = { 1,0 };
    //    mMesh.vertexBufferData[6].uv = { 0,0 };
    //    mMesh.vertexBufferData[4].uv = { 0,1 };


    //    //front face
    //    mMesh.vertexBufferData[1].uv = { 0,1 };
    //    mMesh.vertexBufferData[3].uv = { 0,0 };
    //    mMesh.vertexBufferData[5].uv = { 1,1 };
    //    mMesh.vertexBufferData[7].uv = { 1,0 };

    //}

    size_t sizeOfVertexBuffer = sizeof(std::vector<Vertex>) + (sizeof(Vertex) * this->mMesh.vertexBufferData.size());
    this->vertex = Buffer(sizeOfVertexBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, this->mMesh.vertexBufferData.data());
    size_t sizeOfIndexBuffer = sizeof(std::vector<uint16_t>) + (sizeof(uint16_t) * this->mMesh.indexBufferData.size());
    this->index = Buffer(sizeOfIndexBuffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, this->mMesh.indexBufferData.data());



}

void LoadMeshOBJ(const std::string& path, Mesh& mesh) 
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str())) {
        throw std::runtime_error(warn + err);
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

            vert.uv = 
            {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1 - attrib.texcoords[2 * index.texcoord_index + 1]
            };


            if (uniqueVertices.count(vert) == 0) 
            {
                uniqueVertices[vert] = static_cast<uint32_t>(mesh.vertexBufferData.size());
                mesh.vertexBufferData.push_back(vert);
            }

            mesh.indexBufferData.push_back(uniqueVertices[vert]);





        }

    }

}



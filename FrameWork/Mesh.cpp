#include "Mesh.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_map>

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

    size_t sizeOfVertexBuffer = (sizeof(Vertex) * this->mMesh.vertexBufferData.size());
    this->vertex = Buffer(sizeOfVertexBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, this->mMesh.vertexBufferData.data());
    size_t sizeOfIndexBuffer = (sizeof(int) * this->mMesh.indexBufferData.size());
    this->index = Buffer(sizeOfIndexBuffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, this->mMesh.indexBufferData.data());



}

//boiler plate from framework of graphics class
void LoadMeshOBJ(const std::string& path, Mesh& mesh)
{

    std::ifstream file;

    std::vector<glm::vec2> uvData;
    std::vector<uint16_t> uvIndices;

    file.open(path, std::ios::in);

    if (!file) {
        std::cout << "had an error opening file!\n";
        return;
    }
    std::string line;

    while (std::getline(file, line))
    {
        std::istringstream lineSStream(line);
        std::string type;
        lineSStream >> type;

        if (type == "v") {

            float x, y, z;

            lineSStream >> x >> y >> z;

            Vertex vert = { glm::vec3(x,y,z), glm::vec3(0), glm::vec2(-1, -1) };

            mesh.vertexBufferData.push_back(vert);
        }
        else if (type == "f") //this will only work for files that have delimiters '/' for their faces.
        {
            std::string str_f;
            while (lineSStream >> str_f)
            {
                std::istringstream ref(str_f);
                std::string vStr;
                std::getline(ref, vStr, '/');
                int v = atoi(vStr.c_str()) - 1; //vertex index
                mesh.indexBufferData.push_back(v);
                std::getline(ref, vStr, '/'); //vertex texture index
                uvIndices.push_back(atoi(vStr.c_str()) - 1);
                std::getline(ref, vStr, '/'); //vertex normals index
            }
        }
        else if (type == "vt")
        {
            float u, v;

            lineSStream >> u >> v;
            uvData.push_back({ u,v });
        }
    }

    //now see if the texture coordinates are different.
    size_t original_vertex_buffer_size = mesh.vertexBufferData.size();

    for (size_t i = 0; i < uvIndices.size(); ++i)
    {
        uint16_t& vertIndex = mesh.indexBufferData[i];

        if (mesh.vertexBufferData[vertIndex].uv.x < 0)
        {
            mesh.vertexBufferData[vertIndex].uv = uvData[uvIndices[i]];
        }
        else
        {
            if (mesh.vertexBufferData[vertIndex].uv != uvData[uvIndices[i]])
            {
                bool found = false;
                for (size_t j = original_vertex_buffer_size; j < mesh.vertexBufferData.size(); ++j)
                {
                    if (mesh.vertexBufferData[j].pos == mesh.vertexBufferData[vertIndex].pos && 
                        mesh.vertexBufferData[j].uv == uvData[uvIndices[i]])
                    {
                        found = true;
                        vertIndex = static_cast<uint16_t>(j);
                        break;
                    }
                }

                if (!found)
                {
                    mesh.vertexBufferData.push_back(mesh.vertexBufferData[vertIndex]);
                    mesh.indexBufferData[i] = static_cast<uint16_t>(mesh.vertexBufferData.size() - 1);
                    mesh.vertexBufferData.back().uv = uvData[uvIndices[i]];
                }
            }
        }
    }

    file.close();
}



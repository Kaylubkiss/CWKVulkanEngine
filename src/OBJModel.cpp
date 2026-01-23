#include "OBJModel.h"
#include <glm/glm.hpp>
#include "ApplicationGlobal.h"
#include <algorithm>
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

void OBJModel::ComputeVertexNormals(std::vector<Vertex>& vertexBuffer, std::vector<uint16_t>& indexBuffer)
{
    for (int i = 0; i < vertexBuffer.size(); ++i)
    {
        glm::vec3 total_vec(0.0f);

        for (int j = 0; j < indexBuffer.size(); ++j)
        {
            //total_vec
            if (indexBuffer[j] == i)
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
                    orientation_QP = vertexBuffer[indexBuffer[j + 1]].pos - vertexBuffer[indexBuffer[j]].pos;
                    orientation_RP = vertexBuffer[indexBuffer[j + 2]].pos - vertexBuffer[indexBuffer[j]].pos;

                    edge_1 = orientation_QP;
                    edge_2 = orientation_RP;
                }
                else if (j % 3 == 1) //middle of the face index
                {
                    orientation_QP = vertexBuffer[indexBuffer[j]].pos - vertexBuffer[indexBuffer[j - 1]].pos;
                    orientation_RP = vertexBuffer[indexBuffer[j + 1]].pos - vertexBuffer[indexBuffer[j - 1]].pos;

                    edge_1 = vertexBuffer[indexBuffer[j - 1]].pos - vertexBuffer[indexBuffer[j]].pos;
                    edge_2 = vertexBuffer[indexBuffer[j + 1]].pos - vertexBuffer[indexBuffer[j]].pos;

                }
                else if (j % 3 == 2) //end of face index sequence
                {
                    orientation_QP = vertexBuffer[indexBuffer[j - 1]].pos - vertexBuffer[indexBuffer[j - 2]].pos;
                    orientation_RP = vertexBuffer[indexBuffer[j]].pos - vertexBuffer[indexBuffer[j - 2]].pos;

                    edge_1 = vertexBuffer[indexBuffer[j - 2]].pos - vertexBuffer[indexBuffer[j]].pos;
                    edge_2 = vertexBuffer[indexBuffer[j - 1]].pos - vertexBuffer[indexBuffer[j]].pos;
                }


                angle = glm::degrees(acos((abs(glm::dot(edge_1, edge_2)) /
                    (glm::length(edge_1) * glm::length(edge_2)))));
                normal = glm::cross(orientation_QP, orientation_RP);
                //The angle needs to be between the edges that *SHARE* the vertex.
                total_vec += (angle * normal);
            }
        }

        vertexBuffer[i].nrm = glm::normalize(total_vec); //point + vector equals another point
    }
}

void OBJModel::ComputeVertices(std::vector<Vertex>& vertexBuffer, std::vector<uint16_t>& indexBuffer)
{
    int numVertices = static_cast<int>(vertexBuffer.size());

    glm::vec3 min_points(0.f);
    glm::vec3 max_points(0.f);

    for (unsigned i = 0; i < vertexBuffer.size(); ++i)
    {

        min_points.x = std::min(min_points.x, vertexBuffer[i].pos.x);
        min_points.y = std::min(min_points.y, vertexBuffer[i].pos.y);
        min_points.z = std::min(min_points.z, vertexBuffer[i].pos.z);

        max_points.x = std::max(max_points.x, vertexBuffer[i].pos.x);
        max_points.y = std::max(max_points.y, vertexBuffer[i].pos.y);
        max_points.z = std::max(max_points.z, vertexBuffer[i].pos.z);

        vertexBuffer[i].nrm = glm::vec3(0, 0, 0.f);

        m_center += vertexBuffer[i].pos;
    }

    m_center /= vertexBuffer.size();

    float unitScale = std::max({ glm::length(max_points.x - min_points.x), glm::length(max_points.y - min_points.y), glm::length(max_points.z - min_points.z) });

    max_points = { -std::numeric_limits<float>::min(),  -std::numeric_limits<float>::min() , -std::numeric_limits<float>::min() };
    min_points = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };

    for (size_t i = 0; i < vertexBuffer.size(); ++i)
    {
        vertexBuffer[i].pos = (vertexBuffer[i].pos - m_center) / unitScale;

        max_points.x = std::max(max_points.x, vertexBuffer[i].pos.x);
        max_points.y = std::max(max_points.y, vertexBuffer[i].pos.y);
        max_points.z = std::max(max_points.z, vertexBuffer[i].pos.z);

        min_points.x = std::min(min_points.x, vertexBuffer[i].pos.x);
        min_points.y = std::min(min_points.y, vertexBuffer[i].pos.y);
        min_points.z = std::min(min_points.z, vertexBuffer[i].pos.z);
    }

    m_maxLocalPoint = max_points;
    m_minLocalPoint = min_points;

    OBJModel::ComputeVertexNormals(vertexBuffer, indexBuffer);

   
}

OBJModel::OBJModel(vk::Device* device, const std::filesystem::path& filePath)
{
    std::vector<Vertex> vertexBuffer;
    std::vector<uint16_t> indexBuffer;

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filePath.string().c_str())) {
        throw std::runtime_error(err + ": OBJModel() failed!");
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
                uniqueVertices[vert] = static_cast<uint32_t>(vertexBuffer.size());
                vertexBuffer.push_back(vert);
            }

            indexBuffer.push_back(uniqueVertices[vert]);

        }
    }

    ComputeVertices(vertexBuffer, indexBuffer);

    size_t sizeOfVertexBuffer = (sizeof(vertexBuffer[0]) * vertexBuffer.size());
    m_vertexBuffer = 
        vk::Buffer(device, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 
        sizeOfVertexBuffer, vertexBuffer.data());

    size_t sizeOfIndexBuffer = (sizeof(indexBuffer[0]) * indexBuffer.size());
    m_indexBuffer = 
        vk::Buffer(device, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 
        sizeOfIndexBuffer, indexBuffer.data());

    Primitive primitive;
    primitive.indexCount = static_cast<uint32_t>(indexBuffer.size());
    primitive.vertexCount = static_cast<uint32_t>(vertexBuffer.size());
    
    std::vector<Primitive> primitive_vector = { primitive };

    m_meshes.push_back(std::make_shared<Mesh>(filePath.string(), primitive_vector));

    std::cout << std::endl;
    std::cout << "Model loaded... " + filePath.string() << std::endl;
}

glm::vec3 OBJModel::GetMinPoint() const
{
    return {m_modelTransform * glm::vec4(m_minLocalPoint, 1)};
}

glm::vec3 OBJModel::GetMaxPoint() const
{
    return {m_modelTransform * glm::vec4(m_maxLocalPoint, 1)};
}

void OBJModel::UpdateModelTransform(const glm::mat4& newModelMatrix) 
{
    m_modelTransform = newModelMatrix;
}

void OBJModel::Draw(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout)
{
    if (pipelineLayout != VK_NULL_HANDLE)
    {
        vkCmdPushConstants(cmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), (void*)(&m_modelTransform));
    }

    VkDeviceSize offsets[1] = { 0 };

    VkBuffer vertexBufferHandle = m_vertexBuffer.GetHandle();
    VkBuffer indexBufferHandle = m_indexBuffer.GetHandle();

    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBufferHandle, offsets);
    vkCmdBindIndexBuffer(cmdBuffer, indexBufferHandle, 0, VK_INDEX_TYPE_UINT16);

    // NOTE:.obj does not have any hierarchical structure
    //meaning: m_meshes.size() == 1, primitives.size() == 1
    for (auto& mesh : m_meshes) 
    {
        for (auto& primitive : mesh->m_primitives)
        {
            vkCmdDrawIndexed(cmdBuffer, primitive.indexCount, 1, primitive.firstIndex, primitive.firstVertex, 0);
        }
    }
}
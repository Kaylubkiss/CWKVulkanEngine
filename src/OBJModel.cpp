#include "OBJModel.h"
#include <glm/glm.hpp>
#include "ApplicationGlobal.h"
#define TINYOBJLOADER_DISABLE_FAST_FLOAT
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

void OBJModel::ComputeVertexNormals( std::vector<Vertex>& vertexBuffer, std::vector<uint16_t>& indexBuffer )
{
    //NOTE: assumes that faces are triangulated.

    for (auto& vertex : vertexBuffer)
    {
        vertex.nrm = {};
    }

    for (size_t index = 0; index < indexBuffer.size(); index += 3)
    {
        const int i0 = indexBuffer[index];
        const int i1 = indexBuffer[index + 1];
        const int i2 = indexBuffer[index + 2];

        Vertex& v0 = vertexBuffer[i0];
        Vertex& v1 = vertexBuffer[i1]; //take this center as reference for the edges
        Vertex& v2 = vertexBuffer[i2];

        //should be clockwise... then the normals would point toward the viewer of the faces.
        glm::vec3 E0 = vertexBuffer[i1].pos - vertexBuffer[i0].pos;
        glm::vec3 E1 = vertexBuffer[i2].pos - vertexBuffer[i0].pos;

        glm::vec3 faceNormal = glm::normalize(glm::cross(E0, E1));

        glm::vec3 e00 =  glm::normalize(vertexBuffer[i1].pos - vertexBuffer[i0].pos);
        glm::vec3 e01 = glm::normalize(vertexBuffer[i2].pos - vertexBuffer[i0].pos);
        float alpha0 = acosf(glm::clamp(glm::dot(e00, e01), -1.f, 1.f));

        glm::vec3 e10 = glm::normalize(vertexBuffer[i0].pos - vertexBuffer[i1].pos);
        glm::vec3 e11 = glm::normalize(vertexBuffer[i2].pos - vertexBuffer[i1].pos);
        float alpha1 = acosf(glm::clamp(glm::dot(e10, e11), -1.f, 1.f));


        glm::vec3 e20 = -e11;
        glm::vec3 e21 = -e01;
        float alpha2 = acosf(glm::clamp(glm::dot(e20, e21), -1.f, 1.f));

        v0.nrm += faceNormal * alpha0;
        v1.nrm += faceNormal * alpha1;
        v2.nrm += faceNormal * alpha2;
    }


    //TODO: not optimized
    std::vector<glm::vec3> merged_normals(vertexBuffer.size(), {0.0f, 0.0f, 0.0f});

    //slowly looking for duplicates of the same vertex.
    for (size_t i = 0; i < vertexBuffer.size(); ++i)
    {
        glm::vec3 accumulated_normal = vertexBuffer[i].nrm;
        const glm::vec3& v_pos = vertexBuffer[i].pos;

        for (size_t j = 0; j < vertexBuffer.size(); ++j)
        {
            if (i != j && vertexBuffer[j].pos == v_pos)
            {
                accumulated_normal += vertexBuffer[j].nrm;
            }
        }

        merged_normals[i] = accumulated_normal;
    }

    for (size_t i = 0; i < merged_normals.size(); ++i)
    {
        vertexBuffer[i].nrm = merged_normals[i];
        if (glm::length2(vertexBuffer[i].nrm) > 0.f)
        {
            vertexBuffer[i].nrm = glm::normalize(vertexBuffer[i].nrm);
        }
    }


}

void OBJModel::ComputeVertices( std::vector<Vertex>& vertexBuffer, std::vector<uint16_t>& indexBuffer )
{

    glm::vec3 min_points(0.f);
    glm::vec3 max_points(0.f);

    for (size_t i = 0; i < vertexBuffer.size(); ++i)
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

    float unitScale = std::max({ glm::length(max_points.x - min_points.x),
        glm::length(max_points.y - min_points.y), glm::length(max_points.z - min_points.z) });

    max_points = { };
    min_points = { };

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
}

OBJModel::OBJModel( vk::Device* device, const std::filesystem::path& filePath )
{
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;

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

            if (uniqueVertices.contains(vert) == false)
            {
                uniqueVertices[vert] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vert);
            }

            indices.push_back(uniqueVertices[vert]);
        }
    }

    ComputeVertices(vertices, indices);

    ComputeVertexNormals(vertices, indices);

    size_t sizeOfVertexBuffer = (sizeof(vertices[0]) * vertices.size());
    size_t sizeOfIndexBuffer = (sizeof(indices[0]) * indices.size());

    auto& vertexBuffer = GetVertexBuffer();
    auto& indexBuffer = GetIndexBuffer();

    vertexBuffer =
        vk::Buffer(device, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 
        sizeOfVertexBuffer, vertices.data());

    indexBuffer =
        vk::Buffer(device, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 
        sizeOfIndexBuffer, indices.data());

    Primitive primitive;
    primitive.indexCount = static_cast<uint32_t>(indices.size());
    primitive.vertexCount = static_cast<uint32_t>(vertices.size());
    primitive.textureSetLayoutIndex = 0;
    
    std::vector<Primitive> primitive_vector = { primitive };

    AddMesh(std::make_shared<Mesh>(filePath.string(), primitive_vector));

    std::cout << std::endl;
    std::cout << "Model loaded... " + filePath.string() << std::endl;
}

glm::vec3 OBJModel::GetMinPoint() const
{
    return {GetModelTransform() * glm::vec4(m_minLocalPoint, 1)};
}

glm::vec3 OBJModel::GetMaxPoint() const
{
    return {GetModelTransform() * glm::vec4(m_maxLocalPoint, 1)};
}

void OBJModel::Draw( const vk::DrawInfo& drawInfo )
{
    if (drawInfo.pipelineLayout != VK_NULL_HANDLE)
    {
        vkCmdPushConstants(drawInfo.cmdBuffer, drawInfo.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
            0, sizeof(glm::mat4), (void*)&GetModelTransform());
    }

    VkDeviceSize offsets[1] = { 0 };

    auto& vertexBuffer = GetVertexBuffer();
    auto& indexBuffer = GetIndexBuffer();

    VkBuffer vertexBufferHandle = vertexBuffer.GetHandle();
    VkBuffer indexBufferHandle = indexBuffer.GetHandle();

    vkCmdBindVertexBuffers(drawInfo.cmdBuffer, 0, 1, &vertexBufferHandle, offsets);
    vkCmdBindIndexBuffer(drawInfo.cmdBuffer, indexBufferHandle, 0, VK_INDEX_TYPE_UINT16);

    // NOTE:.obj does not have any hierarchical structure
    //meaning: m_meshes.size() == 1, primitives.size() == 1
    //this written out so that the difference between obj model and gltf get smaller overtime.
    //ideally, Draw() would just be a generic function under IModel.

    auto& meshes = GetMeshes();

    for (auto& mesh : meshes)
    {
        DrawMeshPrimitives(drawInfo, mesh->m_primitives);
    }
}

void OBJModel::LoadTextures( TextureManager& textureManager, const std::vector<std::string>& textureNames )
{
    //OBJ is assumed to only contain one primitive.
    //Materials are not supported with this implementation,
    //this code-base will treat .obj as a primitive format for only geometry and color texture data.
    Mesh& mesh = *GetMeshes().back();
    Primitive& primitive = mesh.m_primitives.back();
    primitive.textureSetLayoutIndex = textureManager.AddTextures( textureNames );
}
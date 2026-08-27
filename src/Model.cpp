//
// Created by cwkmi on 7/29/2026.
//

#include "Model.h"
#include "vkBuffer.h"

glm::vec3 Model::GetMinPoint() const
{
    return { 0.0f, 0.0f, 0.0f };
}

glm::vec3 Model::GetMaxPoint() const
{
    return { 0.0f, 0.0f, 0.0f };
}

const std::vector<Node>& Model::GetNodes()
{
    return m_nodes;
}

const std::vector<size_t>& Model::GetSceneNodeIndices()
{
    return m_sceneNodeIndices;
}

const std::vector<std::shared_ptr<Mesh>>& Model::GetMeshes()
{
    return m_meshes;
}

void Model::AddMesh( const std::shared_ptr<Mesh>& mesh )
{
    m_meshes.push_back(mesh);
}

void Model::AddNode( const Node& node )
{
    m_nodes.push_back(node);
}

void Model::AddSceneNodeIndex(size_t nodeIndex)
{
    m_sceneNodeIndices.push_back(nodeIndex);
}

void Model::AddTextureName(const std::string& textureName)
{
    m_textureNames.push_back(textureName);
}

const glm::mat4& Model::GetModelTransform() const
{
    return m_modelTransform;
}

//for now, assume we only have one physics component for an entire hierarchy of meshes.\
Obviously, there will need to be an overhaul with this.
void Model::UpdateModelTransform( const glm::mat4& newModelMatrix )
{
    m_modelTransform = newModelMatrix;
}

const std::vector<std::string>& Model::GetTextureNames() const
{
    return m_textureNames;
}

void Model::DrawMeshPrimitives( const vk::DrawInfo& drawInfo, const std::vector<Primitive>& primitives )
{
    for (auto& primitive : primitives)
    {
        VkDeviceSize descriptorBufferOffset =
            primitive.textureSetLayoutIndex * drawInfo.textureBindingSize;

        g_vkCmdSetDescriptorBufferOffsetsEXT(drawInfo.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                   drawInfo.pipelineLayout, drawInfo.firstSet, drawInfo.setCount,
                   &drawInfo.imageBufferIndex, &descriptorBufferOffset);

        vkCmdDrawIndexed(drawInfo.cmdBuffer, primitive.indexCount, 1,
            primitive.firstIndex, primitive.firstVertex, 0); //indexing into 1 vertex buffer.
    }
}

vk::Buffer& Model::GetVertexBuffer()
{
    return m_vertexBuffer;
}
vk::Buffer& Model::GetIndexBuffer()
{
    return m_indexBuffer;
}
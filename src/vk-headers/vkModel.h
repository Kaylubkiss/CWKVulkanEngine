#pragma once
#include "IModel.h"
#include "vkBuffer.h"

namespace vk
{
    class Model : public IModel
    {
    public:
        Model() = default;
        Model( vk::Device* device, const std::filesystem::path& filePath ) {}; //NOTE: this makes Model() = default; required.
        ~Model() override
        {
            m_vertexBuffer.Destroy();
            m_indexBuffer.Destroy();
        };
    protected:
        static void DrawMeshPrimitives( const vk::DrawInfo& drawInfo, const std::vector<Primitive>& primitives )
        {
            for (auto& primitive : primitives)
            {
                VkDeviceSize descriptorBufferOffset =
                    primitive.textureSetLayoutIndex * drawInfo.textureBindingSize;

                g_vkCmdSetDescriptorBufferOffsetsEXT(drawInfo.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                           drawInfo.pipelineLayout, drawInfo.firstSet, drawInfo.setCount,
                           &drawInfo.imageBufferIndex, &descriptorBufferOffset);

                vkCmdDrawIndexed(drawInfo.cmdBuffer, primitive.indexCount, 1,
                    primitive.firstIndex, 0, 0); //indexing into 1 vertex buffer.
            }
        }
    protected:
        vk::Buffer m_vertexBuffer;
        vk::Buffer m_indexBuffer;
        std::vector<std::shared_ptr<Mesh>> m_meshes;
    };

}




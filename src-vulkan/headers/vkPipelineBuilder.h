#ifndef VK_PIPELINEBUILDER_HPP
#define VK_PIPELINEBUILDER_HPP

namespace vk
{
    class PipelineBuilder
    {
    public:
        PipelineBuilder() = delete;
        PipelineBuilder( VkPipelineLayout pipelineLayout, VkRenderPass renderPass );

        PipelineBuilder( const PipelineBuilder& other ) = delete;
        PipelineBuilder& operator=( const PipelineBuilder& other ) = delete;

        PipelineBuilder( PipelineBuilder&& other ) noexcept;
        PipelineBuilder& operator=( PipelineBuilder&& other ) noexcept;

        ~PipelineBuilder() = default;

        const std::vector<ShaderModuleInfo>& GetShaderModules() const;
        std::vector<ShaderModuleInfo>& GetShaderModules();

        //depth image
        PipelineBuilder& EnableDepthTest();
        PipelineBuilder& EnableDepthWrite();
        PipelineBuilder& SetDepthCompareOP( VkCompareOp compareOp );

        //vertex binding
        PipelineBuilder& EnableVertexAttributeBinding();

        //shader module
        PipelineBuilder& AddModule( ShaderModuleInfo&& shaderModuleInfo );

        //input assembly
        PipelineBuilder& SetPrimitiveTopology( VkPrimitiveTopology topology );

        //rasterization
        PipelineBuilder& SetCullMode( VkCullModeFlagBits cullMode );

        //color blending
        PipelineBuilder& SetBlendAttachmentCount( uint32_t count );

        //for hot-reloading and resizing the framebuffer
        void UpdateRenderPass( VkRenderPass renderpass );

        void CreatePipeline( VkDevice device, VkPipeline* pipeline );
    private:
        std::vector<ShaderModuleInfo> m_shaderModules;

        VkPipelineRasterizationStateCreateInfo m_rasterizationStateCI = {};
        VkPipelineInputAssemblyStateCreateInfo m_inputAssemblyStateCI = {};

        uint32_t m_blendAttachmentCount = 1;

        //depth information
        VkBool32 m_depthTestEnable = VK_FALSE;
        VkBool32 m_depthWriteEnable = VK_FALSE;
        VkBool32 m_vertexBindingAttributeEnable = VK_FALSE;

        VkCompareOp m_depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

        //these resources need to be carefully handled; not deleted by this class'
        //destruction method.
        VkRenderPass c_renderPass = VK_NULL_HANDLE;
        VkPipelineLayout c_pipelineLayout = VK_NULL_HANDLE;
    };

}

#endif
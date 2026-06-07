//
// Created by cwkmi on 6/5/2026.
//

#include "vkPipelineBuilder.h"
#include "vkInit.h"

namespace vk
{

    PipelineBuilder::PipelineBuilder( VkPipelineLayout pipelineLayout, VkRenderPass renderPass )
    {
    	c_pipelineLayout = pipelineLayout;
    	c_renderPass = renderPass;

        m_inputAssemblyStateCI =
			vk::init::PipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0,
				VK_FALSE);

		m_rasterizationStateCI =
			vk::init::PipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE,
				VK_FRONT_FACE_COUNTER_CLOCKWISE);
    }

	PipelineBuilder::PipelineBuilder( PipelineBuilder&& other ) noexcept
	{
    	c_pipelineLayout = other.c_pipelineLayout;
    	c_renderPass = other.c_renderPass;

    	m_shaderModules = std::move(other.m_shaderModules);

    	m_inputAssemblyStateCI = other.m_inputAssemblyStateCI;
    	m_rasterizationStateCI = other.m_rasterizationStateCI;

    	m_blendAttachmentCount = other.m_blendAttachmentCount;

    	m_depthTestEnable = other.m_depthTestEnable;
    	m_depthWriteEnable = other.m_depthWriteEnable;
    	m_vertexBindingAttributeEnable = other.m_vertexBindingAttributeEnable;

    	m_depthCompareOp = other.m_depthCompareOp;

    	other.c_pipelineLayout = VK_NULL_HANDLE;
    	other.c_renderPass = VK_NULL_HANDLE;
    }

	PipelineBuilder& PipelineBuilder::operator=( PipelineBuilder&& other ) noexcept
	{
    	if ( this != &other )
    	{
			c_pipelineLayout = other.c_pipelineLayout;
    		c_renderPass = other.c_renderPass;

    		m_shaderModules = std::move(other.m_shaderModules);

    		m_inputAssemblyStateCI = other.m_inputAssemblyStateCI;
    		m_rasterizationStateCI = other.m_rasterizationStateCI;

    		m_blendAttachmentCount = other.m_blendAttachmentCount;

    		m_depthTestEnable = other.m_depthTestEnable;
    		m_depthWriteEnable = other.m_depthWriteEnable;
    		m_vertexBindingAttributeEnable = other.m_vertexBindingAttributeEnable;

    		m_depthCompareOp = other.m_depthCompareOp;

    		other.c_pipelineLayout = VK_NULL_HANDLE;
    		other.c_renderPass = VK_NULL_HANDLE;
    	}

    	return *this;
	}

	const std::vector<ShaderModuleInfo>& PipelineBuilder::GetShaderModules() const
	{
		return m_shaderModules;
    }

	std::vector<ShaderModuleInfo>& PipelineBuilder::GetShaderModules()
	{
		return m_shaderModules;
    }

	PipelineBuilder& PipelineBuilder::AddModule( ShaderModuleInfo&& shaderModuleInfo )
	{
		m_shaderModules.push_back( std::move(shaderModuleInfo) );

    	return *this;
    }

	PipelineBuilder& PipelineBuilder::EnableDepthTest()
	{
		m_depthTestEnable = VK_TRUE;

    	return *this;
    }

	PipelineBuilder& PipelineBuilder::EnableDepthWrite()
	{
		m_depthWriteEnable = VK_TRUE;

    	return *this;
	}

	PipelineBuilder& PipelineBuilder::SetDepthCompareOP( VkCompareOp compareOp )
	{
		m_depthCompareOp = compareOp;

    	return *this;
	}

	PipelineBuilder& PipelineBuilder::EnableVertexAttributeBinding()
	{
    	m_vertexBindingAttributeEnable = VK_TRUE;

    	return *this;
    }

    PipelineBuilder& PipelineBuilder::SetPrimitiveTopology( VkPrimitiveTopology topology )
    {
    	m_inputAssemblyStateCI.topology = topology;

        return *this;
    }

    PipelineBuilder& PipelineBuilder::SetCullMode( VkCullModeFlagBits cullMode )
	{
		m_rasterizationStateCI.cullMode = cullMode;

    	return *this;
    }

    PipelineBuilder& PipelineBuilder::SetBlendAttachmentCount( uint32_t count )
	{
    	m_blendAttachmentCount = count;

    	return *this;
    }

	void PipelineBuilder::UpdateRenderPass( VkRenderPass renderpass )
    {
    	c_renderPass = renderpass;
    }

	void PipelineBuilder::CreatePipeline( VkDevice device, VkPipeline* pipeline )
	{
		if ( pipeline != nullptr )
		{
			VkGraphicsPipelineCreateInfo pipelineCI =
				vk::init::PipelineCreateInfo(c_pipelineLayout, c_renderPass,
					VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT);

			pipelineCI.pInputAssemblyState = &m_inputAssemblyStateCI;
			pipelineCI.pRasterizationState = &m_rasterizationStateCI;

			//color blending
			std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentState(m_blendAttachmentCount);
			for ( auto& attachment : blendAttachmentState )
			{
				attachment = vk::init::PipelineColorBlendAttachmentState(0xf, VK_FALSE);
			}

			VkPipelineColorBlendStateCreateInfo colorBlendCI =
				vk::init::PipelineColorBlendStateCreateInfo(
					static_cast<uint32_t>(blendAttachmentState.size()),
					blendAttachmentState.data()
					);

			pipelineCI.pColorBlendState = &colorBlendCI;

			//depth stencil state
			VkPipelineDepthStencilStateCreateInfo depthStencilStateCI =
				vk::init::PipelineDepthStencilStateCreateInfo(m_depthTestEnable, m_depthWriteEnable, m_depthCompareOp);

			pipelineCI.pDepthStencilState = &depthStencilStateCI;

			//multisampling
			VkPipelineMultisampleStateCreateInfo multiplesampleStateCI =
				vk::init::PipelineMultisampleCreateInfo(VK_SAMPLE_COUNT_1_BIT);

			pipelineCI.pMultisampleState = &multiplesampleStateCI;

			//dynamic state
			std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

			VkPipelineDynamicStateCreateInfo dynamicStateCI =
				vk::init::PipelineDynamicStateCreateInfo(dynamicStates);

			pipelineCI.pDynamicState = &dynamicStateCI;

			//viewport state
			VkPipelineViewportStateCreateInfo viewportStateCI =
				vk::init::PipelineViewportStateCreateInfo(1, 1);

			pipelineCI.pViewportState = &viewportStateCI;

			//vertex input binding attributes

			VkPipelineVertexInputStateCreateInfo vertexInputStateCI = vk::init::PipelineVertexInputStateCreateInfo();
			VkVertexInputBindingDescription vertexBindingDescription = vk::init::VertexInputBindingDescription();;
			auto vertexInputAttributeDescriptions = Vertex::InputAttributeDescriptions();

			if ( m_vertexBindingAttributeEnable == VK_TRUE )
			{
				vertexInputStateCI.pVertexBindingDescriptions = &vertexBindingDescription;
				vertexInputStateCI.vertexBindingDescriptionCount = 1;
				vertexInputStateCI.pVertexAttributeDescriptions = vertexInputAttributeDescriptions.data();
				vertexInputStateCI.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributeDescriptions.size());
			}

			pipelineCI.pVertexInputState = &vertexInputStateCI;

			//shader stages
			std::vector<VkPipelineShaderStageCreateInfo> shaderStages(m_shaderModules.size());
			for ( size_t i = 0 ; i < shaderStages.size(); ++i )
			{
				shaderStages[i] = vk::init::PipelineShaderStageCreateInfo(m_shaderModules[i].GetHandle(),
					m_shaderModules[i].GetShaderStageFlags());
			}
			pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
			pipelineCI.pStages = shaderStages.data();

			VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1,
						&pipelineCI, nullptr, pipeline));
		}
    }

}
#include "vkDeferredRenderer.h"
#include "vkInit.h"

namespace vk
{
    void DeferredRenderer::InitializePipeline()
    {
        InitializePipelineLayouts();

		/////////////////////////////////////////////////////////////
		//pipeline #1: composition stage of deferred shading
		{
        	vk::PipelineBuilder pipelineBuilder(m_graphicsPipelineLayout, framebuffers.deComposition.front().GetRenderPass());
        	vk::Pipeline pipeline;

			vk::ShaderModuleInfo vertShaderInfo = vk::ShaderModuleInfo(c_device.GetDevice(),
				"deferred-render/deComposition.vert", VK_SHADER_STAGE_VERTEX_BIT);
			vk::ShaderModuleInfo fragShaderInfo = vk::ShaderModuleInfo(c_device.GetDevice(),
				"deferred-render/deComposition-PBR.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

        	pipelineBuilder
        		.AddModule(std::move(vertShaderInfo))
        		.AddModule(std::move(fragShaderInfo))
        		.SetCullMode(VK_CULL_MODE_FRONT_BIT)
        		.CreatePipeline( c_device.GetDevice(), &pipeline.handle );

        	pipeline.pipelineBuilder = std::make_unique<PipelineBuilder>(std::move(pipelineBuilder));

			pipelineManager.AddPipeline(dePipeline::COMPOSITION, pipeline);
		}

		/////////////////////////////////////////////////////////////
		//pipeline #2: MRT stage of deferred shading -- outputting to color/textures
		{
        	vk::PipelineBuilder pipelineBuilder(m_graphicsPipelineLayout, framebuffers.deMRT.front().GetRenderPass());

        	vk::Pipeline pipeline;

			vk::ShaderModuleInfo vertShaderInfo = ShaderModuleInfo(c_device.GetDevice(),
				"deferred-render/deMRT.vert", VK_SHADER_STAGE_VERTEX_BIT);
			vk::ShaderModuleInfo fragShaderInfo = ShaderModuleInfo(c_device.GetDevice(),
				"deferred-render/deMRT.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

			pipelineBuilder
        		.AddModule(std::move(vertShaderInfo))
        		.AddModule(std::move(fragShaderInfo))
        		.SetCullMode(VK_CULL_MODE_BACK_BIT)
        		.EnableDepthTest()
        		.EnableDepthWrite()
        		.SetBlendAttachmentCount(RT_COUNT)
        		.EnableVertexAttributeBinding()
        		.CreatePipeline(c_device.GetDevice(), &pipeline.handle);

        	pipeline.pipelineBuilder = std::make_unique<PipelineBuilder>(std::move(pipelineBuilder));

			pipelineManager.AddPipeline(dePipeline::MRT, pipeline);
		}

		/////////////////////////////////////////////////////////////
		//pipeline #3: skybox
		{
        	vk::PipelineBuilder pipelineBuilder(m_graphicsPipelineLayout, framebuffers.deSky.front().GetRenderPass());
        	vk::Pipeline pipeline;

			vk::ShaderModuleInfo vertShaderInfo = vk::ShaderModuleInfo(c_device.GetDevice(), "sky.vert", VK_SHADER_STAGE_VERTEX_BIT);
			vk::ShaderModuleInfo fragShaderInfo = vk::ShaderModuleInfo(c_device.GetDevice(), "sky.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

			pipelineBuilder
        		.AddModule(std::move(vertShaderInfo))
        		.AddModule(std::move(fragShaderInfo))
        		.EnableDepthTest()
        		.CreatePipeline( c_device.GetDevice(), &pipeline.handle );

			pipeline.pipelineBuilder = std::make_unique<PipelineBuilder>(std::move(pipelineBuilder));

			pipelineManager.AddPipeline(dePipeline::SKY, pipeline);
		}

		/////////////////////////////////////////////////////////////
		//pipeline #4: swapchain quad
		{
        	vk::PipelineBuilder pipelineBuilder(m_graphicsPipelineLayout, swapChain.GetRenderPass());

        	vk::Pipeline pipeline;

			vk::ShaderModuleInfo vertShaderInfo = vk::ShaderModuleInfo(c_device.GetDevice(), "deferred-render/quad.vert",
				VK_SHADER_STAGE_VERTEX_BIT);
			vk::ShaderModuleInfo fragShaderInfo = vk::ShaderModuleInfo(c_device.GetDevice(), "deferred-render/quad.frag",
				VK_SHADER_STAGE_FRAGMENT_BIT);

        	pipelineBuilder
        		.AddModule(std::move(vertShaderInfo))
        		.AddModule(std::move(fragShaderInfo))
        		.CreatePipeline( c_device.GetDevice(), &pipeline.handle );

        	pipeline.pipelineBuilder = std::make_unique<PipelineBuilder>(std::move(pipelineBuilder));

			pipelineManager.AddPipeline(dePipeline::SWAPCHAIN, pipeline);
		}

		/////////////////////////////////////////////////////////////
		/*//pipeline #5: deferred shadow mapping
		{
        	vk::PipelineBuilder pipelineBuilder(m_graphicsPipelineLayout, swapChain.GetRenderPass());
        	vk::Pipeline pipeline;

			vk::ShaderModuleInfo vertShaderInfo = ShaderModuleInfo(device.GetDevice(), "deferred-render/deShadow.vert",
				VK_SHADER_STAGE_VERTEX_BIT);
			vk::ShaderModuleInfo geoShaderInfo = ShaderModuleInfo(device.GetDevice(), "deferred-render/deShadow.geom",
				VK_SHADER_STAGE_GEOMETRY_BIT);

			pipelineBuilder
        		.AddModule(std::move(vertShaderInfo))
        		.AddModule(std::move(geoShaderInfo))


			//shadow pass doesn't have color attachments
			colorBlendStateCI.attachmentCount = 0;
			colorBlendStateCI.pAttachments = nullptr;

			//enable depth bias as a dynamic state
			rasterizationStateCI.depthBiasEnable = VK_TRUE;
        	rasterizationStateCI.cullMode = VK_CULL_MODE_BACK_BIT;

			pipelineCI.pDepthStencilState = &depthStencilStateCI;

			dynamicStates.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
			dynamicStateCI = vk::init::PipelineDynamicStateCreateInfo(dynamicStates);

			pipelineCI.renderPass = framebuffers.deShadow[0].GetRenderPass();

			//shadow pass only consumes the position of vertices
			VkVertexInputBindingDescription vertexBindingDescription = vk::init::VertexInputBindingDescription();
			VkVertexInputAttributeDescription vertexPosAttributeDescripton = {};
			vertexPosAttributeDescripton.format = VK_FORMAT_R32G32B32_SFLOAT;
			vertexPosAttributeDescripton.location = 0;
			vertexPosAttributeDescripton.binding = 0;
			vertexPosAttributeDescripton.offset = offsetof(struct Vertex, pos);

			VkPipelineVertexInputStateCreateInfo vertexInputStateCI = vk::init::PipelineVertexInputStateCreateInfo();
			vertexInputStateCI.pVertexBindingDescriptions = &vertexBindingDescription;
			vertexInputStateCI.vertexBindingDescriptionCount = 1;
			vertexInputStateCI.pVertexAttributeDescriptions = &vertexPosAttributeDescripton;
			vertexInputStateCI.vertexAttributeDescriptionCount = 1;

			pipelineCI.pVertexInputState = &vertexInputStateCI;

			VkPipeline shadowPipeline = VK_NULL_HANDLE;
			VK_CHECK_RESULT(vkCreateGraphicsPipelines(device.GetDevice(), VK_NULL_HANDLE, 1, &pipelineCI,
				nullptr, &shadowPipeline));

			pipelineManager.AddPipeline(dePipeline::SHADOW, shadowPipeline, nullptr);
		}*/

    }

}
#include "vkDeferredRenderer.h"
#include "vkInit.h"

namespace vk
{
    void DeferredRenderer::InitializePipeline()
    {
        InitializePipelineLayouts();

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI =
			vk::init::PipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0,
				VK_FALSE);

		VkPipelineRasterizationStateCreateInfo rasterizationStateCI =
			vk::init::PipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT,
				VK_FRONT_FACE_COUNTER_CLOCKWISE);

		VkPipelineColorBlendAttachmentState blendAttachmentState =
			vk::init::PipelineColorBlendAttachmentState(0xf, VK_FALSE);

		VkPipelineColorBlendStateCreateInfo colorBlendStateCI =
			vk::init::PipelineColorBlendStateCreateInfo(1, &blendAttachmentState);

		VkPipelineDepthStencilStateCreateInfo depthStencilStateCI =
			vk::init::PipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE,
				VK_COMPARE_OP_LESS_OR_EQUAL);

		VkPipelineViewportStateCreateInfo viewportStateCI =
			vk::init::PipelineViewportStateCreateInfo(1, 1);

		VkPipelineMultisampleStateCreateInfo multiplesampleStateCI =
			vk::init::PipelineMultisampleCreateInfo(VK_SAMPLE_COUNT_1_BIT);

		std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

		VkPipelineDynamicStateCreateInfo dynamicStateCI =
			vk::init::PipelineDynamicStateCreateInfo(dynamicStates);

		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {};

		VkGraphicsPipelineCreateInfo pipelineCI = vk::init::PipelineCreateInfo(m_graphicsPipelineLayout,
			framebuffers.deComposition[0].renderPass, VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT);
		pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
		pipelineCI.pRasterizationState = &rasterizationStateCI;
		pipelineCI.pColorBlendState = &colorBlendStateCI;
		pipelineCI.pDepthStencilState = &depthStencilStateCI;
		pipelineCI.pMultisampleState = &multiplesampleStateCI;
		pipelineCI.pDynamicState = &dynamicStateCI;
		pipelineCI.pViewportState = &viewportStateCI;
		pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCI.pStages = shaderStages.data();

		VkPipelineVertexInputStateCreateInfo emptyVertexInputStateCI = vk::init::PipelineVertexInputStateCreateInfo();
		pipelineCI.pVertexInputState = &emptyVertexInputStateCI;


		/////////////////////////////////////////////////////////////
		//pipeline #1: composition stage of deferred shading
		{
			vk::ShaderModuleInfo vertShaderInfo = vk::ShaderModuleInfo(device.GetDevice(),
				"deferred-render/deComposition.vert", VK_SHADER_STAGE_VERTEX_BIT);
			vk::ShaderModuleInfo fragShaderInfo = vk::ShaderModuleInfo(device.GetDevice(),
				"deferred-render/deComposition-PBR.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

			shaderStages[0] = vk::init::PipelineShaderStageCreateInfo(vertShaderInfo.GetHandle(), vertShaderInfo.GetShaderStageFlags());
			shaderStages[1] = vk::init::PipelineShaderStageCreateInfo(fragShaderInfo.GetHandle(), fragShaderInfo.GetShaderStageFlags());

			rasterizationStateCI.cullMode = VK_CULL_MODE_FRONT_BIT;

			pipelineManager.AddModule(dePipeline::COMPOSITION, std::move(vertShaderInfo));
			pipelineManager.AddModule(dePipeline::COMPOSITION, std::move(fragShaderInfo));

			VkPipeline lightPassPipeline = VK_NULL_HANDLE;
			VK_CHECK_RESULT(vkCreateGraphicsPipelines(device.GetDevice(), VK_NULL_HANDLE, 1, &pipelineCI,
				nullptr, &lightPassPipeline));

			//for the hot reloading - light pass
			std::function<void()> lightPassCreationFunction =
				[this,
				inputAssemblyStateCI,
				rasterizationStateCI,
				depthStencilStateCI,
				multiplesampleStateCI,
				viewportStateCI,
				emptyVertexInputStateCI]
				{

					VkPipeline pipeline = pipelineManager.Get(dePipeline::COMPOSITION);

					if (pipeline != VK_NULL_HANDLE)
					{
						vkDestroyPipeline(device.GetDevice(), pipeline, nullptr);
						pipeline = VK_NULL_HANDLE;
					}

					VkPipelineColorBlendAttachmentState blendAttachmentState =
						vk::init::PipelineColorBlendAttachmentState(0xf, VK_FALSE);
					VkPipelineColorBlendStateCreateInfo colorBlendStateCI =
						vk::init::PipelineColorBlendStateCreateInfo(1, &blendAttachmentState);

					std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
					VkPipelineDynamicStateCreateInfo dynamicStateCI =
						vk::init::PipelineDynamicStateCreateInfo(dynamicStates);

					std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {};

					VkGraphicsPipelineCreateInfo pipelineCI =
						vk::init::PipelineCreateInfo(
							m_graphicsPipelineLayout, framebuffers.deComposition[0].renderPass,
							VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT);

					pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
					pipelineCI.pRasterizationState = &rasterizationStateCI;
					pipelineCI.pColorBlendState = &colorBlendStateCI;
					pipelineCI.pDepthStencilState = &depthStencilStateCI;
					pipelineCI.pMultisampleState = &multiplesampleStateCI;
					pipelineCI.pDynamicState = &dynamicStateCI;
					pipelineCI.pViewportState = &viewportStateCI;
					pipelineCI.pVertexInputState = &emptyVertexInputStateCI;
					pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
					pipelineCI.pStages = shaderStages.data();

					const auto& shaders =
						pipelineManager.GetPipelineShaders(dePipeline::COMPOSITION);
					shaderStages[0] = vk::init::PipelineShaderStageCreateInfo(shaders[0].GetHandle(), shaders[0].GetShaderStageFlags());
					shaderStages[1] = vk::init::PipelineShaderStageCreateInfo(shaders[1].GetHandle(), shaders[1].GetShaderStageFlags());

					VK_CHECK_RESULT(vkCreateGraphicsPipelines(device.GetDevice(), VK_NULL_HANDLE, 1,
						&pipelineCI, nullptr, &pipeline));

					pipelineManager.AddPipeline(dePipeline::COMPOSITION, pipeline);
				};

			pipelineManager.AddPipeline(dePipeline::COMPOSITION, lightPassPipeline,
				std::move(lightPassCreationFunction));
		}

		/////////////////////////////////////////////////////////////
		//pipeline #2: MRT stage of deferred shading -- outputting to color/textures
		{
			vk::ShaderModuleInfo vertShaderInfo = ShaderModuleInfo(device.GetDevice(),
				"deferred-render/deMRT.vert", VK_SHADER_STAGE_VERTEX_BIT);
			vk::ShaderModuleInfo fragShaderInfo = ShaderModuleInfo(device.GetDevice(),
				"deferred-render/deMRT.frag", VK_SHADER_STAGE_FRAGMENT_BIT);


			shaderStages[0] = vk::init::PipelineShaderStageCreateInfo(vertShaderInfo.GetHandle(), vertShaderInfo.GetShaderStageFlags());
			shaderStages[1] = vk::init::PipelineShaderStageCreateInfo(fragShaderInfo.GetHandle(), fragShaderInfo.GetShaderStageFlags());


			rasterizationStateCI.cullMode = VK_CULL_MODE_BACK_BIT;

			pipelineCI.renderPass = framebuffers.deMRT[0].renderPass;

			//there are 5 color outputs in this stage.
			std::array<VkPipelineColorBlendAttachmentState, RT_COUNT> blendAttachmentStates = {};
			for (auto& attachment : blendAttachmentStates)
			{
				attachment = vk::init::PipelineColorBlendAttachmentState(0xf, VK_FALSE);
			}

			colorBlendStateCI.pAttachments = blendAttachmentStates.data();
			colorBlendStateCI.attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size());


			//reminder: using a single vertex binding, so binding is 0.
			VkVertexInputBindingDescription vertexBindingDescription = vk::init::VertexInputBindingDescription();
			auto vertexInputAttributeDescriptions =
				Vertex::InputAttributeDescriptions();

			VkPipelineVertexInputStateCreateInfo vertexInputStateCI = vk::init::PipelineVertexInputStateCreateInfo();
			vertexInputStateCI.pVertexBindingDescriptions = &vertexBindingDescription;
			vertexInputStateCI.vertexBindingDescriptionCount = 1;
			vertexInputStateCI.pVertexAttributeDescriptions = vertexInputAttributeDescriptions.data();
			vertexInputStateCI.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributeDescriptions.size());

			pipelineCI.pVertexInputState = &vertexInputStateCI;

			pipelineManager.AddModule(dePipeline::MRT, std::move(vertShaderInfo));
			pipelineManager.AddModule(dePipeline::MRT, std::move(fragShaderInfo));

			VkPipeline mrtPipeline = VK_NULL_HANDLE;
			VK_CHECK_RESULT(vkCreateGraphicsPipelines(device.GetDevice(), VK_NULL_HANDLE, 1,
				&pipelineCI, nullptr, &mrtPipeline));

			//for hot reloading - MRT pass
			std::function<void()> MRTPassCreationFunction =
				[this,
				inputAssemblyStateCI,
				rasterizationStateCI,
				depthStencilStateCI,
				multiplesampleStateCI,
				viewportStateCI]
				{

					VkPipeline pipeline = pipelineManager.Get(dePipeline::MRT);

					if (pipeline != VK_NULL_HANDLE)
					{
						vkDestroyPipeline(device.GetDevice(), pipeline, nullptr);
						pipeline = VK_NULL_HANDLE;
					}

					std::array<VkPipelineColorBlendAttachmentState, RT_COUNT> blendAttachmentStates = {};
					for (auto& attachment : blendAttachmentStates)
					{
						attachment = vk::init::PipelineColorBlendAttachmentState(0xf, VK_FALSE);
					}

					VkPipelineColorBlendStateCreateInfo colorBlendStateCI = vk::init::PipelineColorBlendStateCreateInfo
					(
						static_cast<uint32_t>(blendAttachmentStates.size()), blendAttachmentStates.data()
					);

					std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
					VkPipelineDynamicStateCreateInfo dynamicStateCI =
						vk::init::PipelineDynamicStateCreateInfo(dynamicStates);

					VkVertexInputBindingDescription vertexBindingDescription =
						vk::init::VertexInputBindingDescription();
					auto vertexInputAttributeDescriptions =
						Vertex::InputAttributeDescriptions();

					VkPipelineVertexInputStateCreateInfo vertexInputStateCI =
						vk::init::PipelineVertexInputStateCreateInfo();
					vertexInputStateCI.pVertexBindingDescriptions = &vertexBindingDescription;
					vertexInputStateCI.vertexBindingDescriptionCount = 1;
					vertexInputStateCI.pVertexAttributeDescriptions = vertexInputAttributeDescriptions.data();
					vertexInputStateCI.vertexAttributeDescriptionCount =
						static_cast<uint32_t>(vertexInputAttributeDescriptions.size());

					std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

					VkGraphicsPipelineCreateInfo pipelineCI =
						vk::init::PipelineCreateInfo(m_graphicsPipelineLayout, framebuffers.deMRT[0].renderPass,
							VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT);

					pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
					pipelineCI.pRasterizationState = &rasterizationStateCI;
					pipelineCI.pColorBlendState = &colorBlendStateCI;
					pipelineCI.pDepthStencilState = &depthStencilStateCI;
					pipelineCI.pMultisampleState = &multiplesampleStateCI;
					pipelineCI.pDynamicState = &dynamicStateCI;
					pipelineCI.pViewportState = &viewportStateCI;
					pipelineCI.pVertexInputState = &vertexInputStateCI;
					pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
					pipelineCI.pStages = shaderStages.data();

					const std::vector<ShaderModuleInfo>& shaders = pipelineManager.GetPipelineShaders(dePipeline::MRT);
					shaderStages[0] = vk::init::PipelineShaderStageCreateInfo(shaders[0].GetHandle(), shaders[0].GetShaderStageFlags());
					shaderStages[1] = vk::init::PipelineShaderStageCreateInfo(shaders[1].GetHandle(), shaders[1].GetShaderStageFlags());

					VK_CHECK_RESULT(vkCreateGraphicsPipelines(device.GetDevice(), VK_NULL_HANDLE, 1,
						&pipelineCI, nullptr, &pipeline));

					pipelineManager.AddPipeline(dePipeline::MRT, pipeline);
				};


			pipelineManager.AddPipeline(dePipeline::MRT, mrtPipeline, std::move(MRTPassCreationFunction));
		}

		/////////////////////////////////////////////////////////////
		//pipeline #3: skybox
		{
			vk::ShaderModuleInfo vertShaderInfo = vk::ShaderModuleInfo(device.GetDevice(), "sky.vert", VK_SHADER_STAGE_VERTEX_BIT);
			vk::ShaderModuleInfo fragShaderInfo = vk::ShaderModuleInfo(device.GetDevice(), "sky.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

			pipelineManager.AddModule(dePipeline::SKY, std::move(vertShaderInfo));
			pipelineManager.AddModule(dePipeline::SKY, std::move(fragShaderInfo));

			shaderStages[0] = vk::init::PipelineShaderStageCreateInfo(vertShaderInfo.GetHandle(), vertShaderInfo.GetShaderStageFlags());
			shaderStages[1] = vk::init::PipelineShaderStageCreateInfo(fragShaderInfo.GetHandle(), fragShaderInfo.GetShaderStageFlags());

			rasterizationStateCI.cullMode = VK_CULL_MODE_NONE;
			colorBlendStateCI = vk::init::PipelineColorBlendStateCreateInfo(1, &blendAttachmentState);

			//using a full-screen quad method.
			pipelineCI.pVertexInputState = &emptyVertexInputStateCI;
			pipelineCI.renderPass = framebuffers.deSky[0].renderPass;

			VkPipelineDepthStencilStateCreateInfo emptyDepthStencilStateCI =
				vk::init::PipelineDepthStencilStateCreateInfo(VK_TRUE,
					VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);

			pipelineCI.pDepthStencilState = &emptyDepthStencilStateCI;

			VkPipeline skyPipeline = VK_NULL_HANDLE;
			VK_CHECK_RESULT(vkCreateGraphicsPipelines(device.GetDevice(), VK_NULL_HANDLE, 1, &pipelineCI,
				nullptr, &skyPipeline));

        	std::function<void()> skyboxCreateFunc = [
        		this,
        		inputAssemblyStateCI,
        		rasterizationStateCI,
        		emptyDepthStencilStateCI,
        		multiplesampleStateCI,
        		emptyVertexInputStateCI,
        		viewportStateCI]
        	{

        		VkPipeline pipeline = pipelineManager.Get(dePipeline::SKY);

        		if (pipeline != VK_NULL_HANDLE)
        		{
        			vkDestroyPipeline(device.GetDevice(), pipeline, nullptr);
        			pipeline = VK_NULL_HANDLE;
        		}


        		std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        		VkPipelineDynamicStateCreateInfo dynamicStateCI =
					vk::init::PipelineDynamicStateCreateInfo(dynamicStates);

        		VkPipelineColorBlendAttachmentState blendAttachmentState =
					vk::init::PipelineColorBlendAttachmentState(0xf, VK_FALSE);

        		VkPipelineColorBlendStateCreateInfo colorBlendStateCI =
        			vk::init::PipelineColorBlendStateCreateInfo(1, &blendAttachmentState);

        		VkGraphicsPipelineCreateInfo pipelineCI =
						vk::init::PipelineCreateInfo(
							m_graphicsPipelineLayout, framebuffers.deSky[0].renderPass,
							VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT);

        		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

        		pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
        		pipelineCI.pRasterizationState = &rasterizationStateCI;
        		pipelineCI.pColorBlendState = &colorBlendStateCI;
        		pipelineCI.pDepthStencilState = &emptyDepthStencilStateCI;
        		pipelineCI.pMultisampleState = &multiplesampleStateCI;
        		pipelineCI.pDynamicState = &dynamicStateCI;
        		pipelineCI.pViewportState = &viewportStateCI;
        		pipelineCI.pVertexInputState = &emptyVertexInputStateCI;
        		pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
        		pipelineCI.pStages = shaderStages.data();

        		const std::vector<ShaderModuleInfo>& shaders = pipelineManager.GetPipelineShaders(dePipeline::SKY);
        		shaderStages[0] = vk::init::PipelineShaderStageCreateInfo(shaders[0].GetHandle(), shaders[0].GetShaderStageFlags());
        		shaderStages[1] = vk::init::PipelineShaderStageCreateInfo(shaders[1].GetHandle(), shaders[1].GetShaderStageFlags());

        		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device.GetDevice(), VK_NULL_HANDLE, 1,
						&pipelineCI, nullptr, &pipeline));

        		pipelineManager.AddPipeline(dePipeline::SKY, pipeline);
        	};

			pipelineManager.AddPipeline(dePipeline::SKY, skyPipeline, std::move(skyboxCreateFunc));
		}

		/////////////////////////////////////////////////////////////
		//pipeline #4: swapchain quad
		{
			vk::ShaderModuleInfo vertShaderInfo = vk::ShaderModuleInfo(device.GetDevice(), "deferred-render/quad.vert",
				VK_SHADER_STAGE_VERTEX_BIT);

			vk::ShaderModuleInfo fragShaderInfo = vk::ShaderModuleInfo(device.GetDevice(), "deferred-render/quad.frag",
				VK_SHADER_STAGE_FRAGMENT_BIT);

			shaderStages[0] = vk::init::PipelineShaderStageCreateInfo(vertShaderInfo.GetHandle(), vertShaderInfo.GetShaderStageFlags());
			shaderStages[1] = vk::init::PipelineShaderStageCreateInfo(fragShaderInfo.GetHandle(), fragShaderInfo.GetShaderStageFlags());


			pipelineManager.AddModule(dePipeline::SWAPCHAIN, std::move(vertShaderInfo));
			pipelineManager.AddModule(dePipeline::SWAPCHAIN, std::move(fragShaderInfo));

			rasterizationStateCI.cullMode = VK_CULL_MODE_NONE;
			colorBlendStateCI = vk::init::PipelineColorBlendStateCreateInfo(1, &blendAttachmentState);

			pipelineCI.pVertexInputState = &emptyVertexInputStateCI;
			pipelineCI.renderPass = swapChain.renderPass;

			VkPipelineDepthStencilStateCreateInfo emptyDepthStencilStateCI =
				vk::init::PipelineDepthStencilStateCreateInfo(VK_FALSE,
					VK_FALSE, VK_COMPARE_OP_NEVER);

			pipelineCI.pDepthStencilState = &emptyDepthStencilStateCI;

			VkPipeline swapchainPipeline = VK_NULL_HANDLE;

			VK_CHECK_RESULT(vkCreateGraphicsPipelines(device.GetDevice(), VK_NULL_HANDLE, 1, &pipelineCI,
				nullptr, &swapchainPipeline));

			pipelineManager.AddPipeline(dePipeline::SWAPCHAIN, swapchainPipeline, nullptr);
		}

		/////////////////////////////////////////////////////////////
		//pipeline #5: deferred shadow mapping
		{
			vk::ShaderModuleInfo vertShaderInfo = ShaderModuleInfo(device.GetDevice(), "deferred-render/deShadow.vert",
				VK_SHADER_STAGE_VERTEX_BIT);
			vk::ShaderModuleInfo geoShaderInfo = ShaderModuleInfo(device.GetDevice(), "deferred-render/deShadow.geom",
				VK_SHADER_STAGE_GEOMETRY_BIT);

			shaderStages[0] = vk::init::PipelineShaderStageCreateInfo(vertShaderInfo.GetHandle(), vertShaderInfo.GetShaderStageFlags());
			shaderStages[1] = vk::init::PipelineShaderStageCreateInfo(geoShaderInfo.GetHandle(), geoShaderInfo.GetShaderStageFlags());

			pipelineManager.AddModule(dePipeline::SHADOW, std::move(vertShaderInfo));
			pipelineManager.AddModule(dePipeline::SHADOW, std::move(geoShaderInfo));

			//shadow pass doesn't have color attachments
			colorBlendStateCI.attachmentCount = 0;
			colorBlendStateCI.pAttachments = nullptr;

			//enable depth bias as a dynamic state
			rasterizationStateCI.depthBiasEnable = VK_TRUE;
        	rasterizationStateCI.cullMode = VK_CULL_MODE_BACK_BIT;

			pipelineCI.pDepthStencilState = &depthStencilStateCI;

			dynamicStates.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
			dynamicStateCI = vk::init::PipelineDynamicStateCreateInfo(dynamicStates);

			pipelineCI.renderPass = framebuffers.deShadow[0].renderPass;

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
		}

    }

}
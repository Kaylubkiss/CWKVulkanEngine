#include "vkDeferredRenderer.h"
#include "vkInit.h"

namespace vk
{
    void DeferredRenderer::RecordCommandBuffers( const SceneView& sceneView )
	{
		VkCommandBuffer cmdBuffer = commandBuffers[currentFrame];
		VkCommandBufferBeginInfo cmdBufferBeginInfo = vk::init::CommandBufferBeginInfo();

		//clear value count corresponds to the number of attachments.
		VkClearValue clearValues[RT_COUNT + 1]; //position, normal, albedo, metallic roughness, depth;

		VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufferBeginInfo));

    	std::vector<VkDescriptorBufferBindingInfoEXT> descriptorBufferBindingInfos =
    	{
    		{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
    			.address = m_descriptorManagerPtr.GetDescriptorAddress(DescriptorCategory::eUBO),
    			.usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT
			},
    		{
    			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
    			.address = m_descriptorManagerPtr.GetDescriptorAddress(DescriptorCategory::eCompositionImage),
    			.usage = VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT |
					VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT
    		},
    		{
    			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
    			.address = m_descriptorManagerPtr.GetDescriptorAddress(DescriptorCategory::eMaterial),
    			.usage = VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT |
    				VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT
    		}
    	};

    	const uint32_t uboDescriptorIndex = 0, compositionImageDescriptorIndex = 1, materialDescriptorIndex = 2;

    	g_vkCmdBindDescriptorBuffersEXT(cmdBuffer, static_cast<uint32_t>(descriptorBufferBindingInfos.size()),
				descriptorBufferBindingInfos.data());

		/*//Shadow depth writes
		{
			clearValues[0].depthStencil = { 1.0f, 0 };

			VkRenderPassBeginInfo renderPassBI = vk::init::RenderPassBeginInfo();
			renderPassBI.clearValueCount = 1;
			renderPassBI.pClearValues = clearValues;
			renderPassBI.renderArea.extent = { (uint32_t)framebuffers.deShadow[currentFrame].width,
				(uint32_t)framebuffers.deShadow[currentFrame].height };
			renderPassBI.renderPass = framebuffers.deShadow[currentFrame].renderPass;
			renderPassBI.framebuffer = framebuffers.deShadow[currentFrame].handle;

			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineManager.Get(dePipeline::SHADOW));
			vkCmdBeginRenderPass(cmdBuffer, &renderPassBI, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport shadowViewport = vk::init::Viewport(framebuffers.deShadow[currentFrame].width, framebuffers.deShadow[currentFrame].height);
			vkCmdSetViewport(cmdBuffer, 0, 1, &shadowViewport);

			VkRect2D shadowScissor = vk::init::Rect2D(framebuffers.deShadow[currentFrame].width,
				framebuffers.deShadow[currentFrame].height);
			vkCmdSetScissor(cmdBuffer, 0, 1, &shadowScissor);

			vkCmdSetDepthBias(cmdBuffer, depthBiasConstant, 0.f, depthBiasSlope);

			// Binding 0 = uniform buffer
			auto& shadowUniformDescriptor = uniformBindingDescriptors[dePipeline::SHADOW];

			std::array<VkDescriptorBufferBindingInfoEXT, 1> descriptor_buffer_binding_info = {};
			descriptor_buffer_binding_info[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
			descriptor_buffer_binding_info[0].address =
				shadowUniformDescriptor.GetBuffer().GetDeviceAddress();
			descriptor_buffer_binding_info[0].usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;

			g_vkCmdBindDescriptorBuffersEXT(cmdBuffer, static_cast<uint32_t>(descriptor_buffer_binding_info.size()),
				descriptor_buffer_binding_info.data());

			uint32_t buffer_index_ubo = 0;
			VkDeviceSize buffer_offset = currentFrame * shadowUniformDescriptor.GetLayoutSize();

			g_vkCmdSetDescriptorBufferOffsetsEXT(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineLayouts[dePipeline::SHADOW], 0, 1, &buffer_index_ubo, &buffer_offset);

			vk::DrawInfo drawInfo = {};
			drawInfo.cmdBuffer = cmdBuffer;
			drawInfo.pipelineLayout = pipelineLayouts[dePipeline::SHADOW];

			m_assetManager->DrawObjects(drawInfo);

			vkCmdEndRenderPass(cmdBuffer);
		}*/

    	VkDeviceSize uboLayoutSize = m_descriptorManagerPtr.GetLayoutSize(DescriptorCategory::eUBO);

		//MRT rendering.
		{
			clearValues[0].color = { 0,0,0,0 };
			clearValues[1].color = clearValues[0].color;
			clearValues[2].color = clearValues[0].color;
			clearValues[3].color = clearValues[0].color;
			clearValues[4].color = clearValues[0].color;
			clearValues[5].depthStencil = { 1.f, 0 };

			VkRenderPassBeginInfo renderPassBeginInfo = vk::init::RenderPassBeginInfo();
			renderPassBeginInfo.clearValueCount = RT_COUNT + 1;
			renderPassBeginInfo.pClearValues = clearValues;

			VkExtent2D fbExtent = framebuffers.deMRT[currentFrame].GetExtent();
			VkRenderPass fbRenderPass = framebuffers.deMRT[currentFrame].GetRenderPass();
			VkFramebuffer fbHandle = framebuffers.deMRT[currentFrame].GetHandle();

			renderPassBeginInfo.renderArea.extent = { fbExtent.width, fbExtent.height };
			renderPassBeginInfo.renderPass = fbRenderPass;
			renderPassBeginInfo.framebuffer = fbHandle;

			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineManager.Get(dePipeline::MRT));

			vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport sceneViewport =  m_window.Viewport();
			vkCmdSetViewport(cmdBuffer, 0, 1, &sceneViewport);

			VkRect2D sceneScissor = m_window.Scissor();
			vkCmdSetScissor(cmdBuffer, 0, 1, &sceneScissor);


			VkDeviceSize buffer_offset =  (gMaxFramesInFlight * mrtUBOLayoutIndex + currentFrame) * uboLayoutSize;

			//global transform
			g_vkCmdSetDescriptorBufferOffsetsEXT(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				m_graphicsPipelineLayout, uboDescriptorIndex, 1, &uboDescriptorIndex, &buffer_offset);

			vk::DrawInfo drawInfo = {};
			drawInfo.cmdBuffer = cmdBuffer;
			drawInfo.imageBufferIndex = materialDescriptorIndex;
			drawInfo.firstSet = materialDescriptorIndex;
			drawInfo.pipelineLayout = m_graphicsPipelineLayout;
			drawInfo.textureBindingSize = m_descriptorManagerPtr.GetLayoutSize(DescriptorCategory::eMaterial);

			for (auto& object : sceneView.opaqueObjects)
			{
				object->Draw(drawInfo);
			}

			vkCmdEndRenderPass(cmdBuffer);
		}

		//Composition
		{
			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineManager.Get(dePipeline::COMPOSITION));

			VkViewport windowViewport = m_window.Viewport();

			clearValues[0].color = { 0,0,0,0 };
			clearValues[1].depthStencil = { 1.f, 0 };

			VkRenderPassBeginInfo renderPassBeginInfo = vk::init::RenderPassBeginInfo();
			renderPassBeginInfo.clearValueCount = 1;
			renderPassBeginInfo.pClearValues = clearValues;

			VkExtent2D fbExtent = framebuffers.deComposition[currentFrame].GetExtent();
			VkRenderPass fbRenderPass = framebuffers.deComposition[currentFrame].GetRenderPass();
			VkFramebuffer fbHandle = framebuffers.deComposition[currentFrame].GetHandle();

			renderPassBeginInfo.renderArea.extent = fbExtent;
			renderPassBeginInfo.renderPass = fbRenderPass;
			renderPassBeginInfo.framebuffer = fbHandle;

			vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport sceneViewport = windowViewport;
			vkCmdSetViewport(cmdBuffer, 0, 1, &sceneViewport);

			VkRect2D sceneScissor = m_window.Scissor();
			vkCmdSetScissor(cmdBuffer, 0, 1, &sceneScissor);

			//composition image samplers;
			VkDeviceSize buffer_offset = (gMaxFramesInFlight * compositionImageIndex + currentFrame) *
				m_descriptorManagerPtr.GetLayoutSize(DescriptorCategory::eCompositionImage);

			g_vkCmdSetDescriptorBufferOffsetsEXT(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				m_graphicsPipelineLayout, compositionImageDescriptorIndex, 1, &compositionImageDescriptorIndex, &buffer_offset);

			//ubo (lights)
			buffer_offset = (gMaxFramesInFlight * lightUBOLayoutIndex + currentFrame) * uboLayoutSize;

			g_vkCmdSetDescriptorBufferOffsetsEXT(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				m_graphicsPipelineLayout, uboDescriptorIndex, 1, &uboDescriptorIndex, &buffer_offset);

			vkCmdDraw(cmdBuffer, 3, 1, 0, 0);

			vkCmdEndRenderPass(cmdBuffer);
		}

		//Skybox.
		{
			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineManager.Get(dePipeline::SKY));

			VkViewport windowViewport = m_window.Viewport();

			clearValues[0].color = { 0,0,0,0 };
			clearValues[1].depthStencil = { 1.f, 0 };

			VkRenderPassBeginInfo renderPassBeginInfo = vk::init::RenderPassBeginInfo();
			renderPassBeginInfo.clearValueCount = 2;
			renderPassBeginInfo.pClearValues = clearValues;

			VkExtent2D fbExtent = framebuffers.deSky[currentFrame].GetExtent();
			VkRenderPass fbRenderPass = framebuffers.deSky[currentFrame].GetRenderPass();
			VkFramebuffer fbHandle = framebuffers.deSky[currentFrame].GetHandle();

			renderPassBeginInfo.renderArea.extent = fbExtent;
			renderPassBeginInfo.renderPass = fbRenderPass;
			renderPassBeginInfo.framebuffer = fbHandle;

			vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdSetViewport(cmdBuffer, 0, 1, &windowViewport);

			VkRect2D sceneScissor = m_window.Scissor();
			vkCmdSetScissor(cmdBuffer, 0, 1, &sceneScissor);


			VkDeviceSize buffer_offset = (gMaxFramesInFlight * mrtUBOLayoutIndex + currentFrame) * uboLayoutSize;

			//global transform
			g_vkCmdSetDescriptorBufferOffsetsEXT(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				m_graphicsPipelineLayout, uboDescriptorIndex, 1, &uboDescriptorIndex, &buffer_offset);

			buffer_offset = skyboxImageIndex * m_descriptorManagerPtr.GetLayoutSize(DescriptorCategory::eMaterial);

			//scene sampler
			g_vkCmdSetDescriptorBufferOffsetsEXT(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				m_graphicsPipelineLayout,materialDescriptorIndex, 1, &materialDescriptorIndex, &buffer_offset);

			vkCmdDraw(cmdBuffer, 3, 1, 0, 0);

			vkCmdEndRenderPass(cmdBuffer);
		}

    	//render glass/transparent objects here.

		//Swapchain Quad
		{
			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineManager.Get(dePipeline::SWAPCHAIN));

			VkViewport windowViewport = m_window.Viewport();

			clearValues[0].color = { 0,0,0,0 };

			VkRenderPassBeginInfo renderPassBeginInfo = vk::init::RenderPassBeginInfo();
			renderPassBeginInfo.clearValueCount = 1;
			renderPassBeginInfo.pClearValues = clearValues;

			VkExtent2D fbExtent = swapChain.GetExtent();
			VkFramebuffer fbHandle = swapChain.GetFramebuffers()[currentImageIndex];

			renderPassBeginInfo.renderArea.extent = fbExtent;
			renderPassBeginInfo.renderPass = swapChain.GetRenderPass();
			renderPassBeginInfo.framebuffer = fbHandle;


			vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport sceneViewport = windowViewport;
			vkCmdSetViewport(cmdBuffer, 0, 1, &sceneViewport);

			VkRect2D sceneScissor = m_window.Scissor();
			vkCmdSetScissor(cmdBuffer, 0, 1, &sceneScissor);

			VkDeviceSize buffer_offset =  (gMaxFramesInFlight * swapChainImageIndex + currentFrame) *
				m_descriptorManagerPtr.GetLayoutSize(DescriptorCategory::eCompositionImage);

			g_vkCmdSetDescriptorBufferOffsetsEXT(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				m_graphicsPipelineLayout, compositionImageDescriptorIndex, 1, &compositionImageDescriptorIndex, &buffer_offset);

			vkCmdDraw(cmdBuffer, 3, 1, 0, 0);

			if (m_settings.UIDisplay)
			{
				UserInterface::Render(cmdBuffer);
			}

			vkCmdEndRenderPass(cmdBuffer);
		}

		VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));

	}
}
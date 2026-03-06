#include "vkDeferredShadingContext.h"

namespace vk
{
    void DeferredContext::RecordCommandBuffers()
	{
		VkCommandBuffer cmdBuffer = commandBuffers[currentFrame];
		VkCommandBufferBeginInfo cmdBufferBeginInfo = vk::init::CommandBufferBeginInfo();

		//clear value count corresponds to the number of attachments.
		VkClearValue clearValues[RT_COUNT + 1]; //position, normal, albedo, metallic roughness, depth;

		VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufferBeginInfo));

		//Shadow depth writes
		{
			clearValues[0].depthStencil = { 1.0f, 0 };

			VkRenderPassBeginInfo renderPassBI = vk::init::RenderPassBeginInfo();
			renderPassBI.clearValueCount = 1;
			renderPassBI.pClearValues = clearValues;
			renderPassBI.renderArea.extent = { (uint32_t)framebuffers.deShadow.width,
				(uint32_t)framebuffers.deShadow.height };
			renderPassBI.renderPass = framebuffers.deShadow.renderPass;
			renderPassBI.framebuffer = framebuffers.deShadow.handle;

			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineManager.Get(dePipeline::SHADOW));
			vkCmdBeginRenderPass(cmdBuffer, &renderPassBI, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport shadowViewport = vk::init::Viewport(framebuffers.deShadow.width, framebuffers.deShadow.height);
			vkCmdSetViewport(cmdBuffer, 0, 1, &shadowViewport);

			VkRect2D shadowScissor = vk::init::Rect2D(framebuffers.deShadow.width, framebuffers.deShadow.height);
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
		}

    	auto& textureManager = m_assetManager->GetTextureManager();
    	auto& textureSamplerDescriptor = textureManager.GetTextureSamplerDescriptor();
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
			renderPassBeginInfo.renderArea.extent = { (uint32_t)framebuffers.deMRT.width,
				(uint32_t)framebuffers.deMRT.height };
			renderPassBeginInfo.renderPass = framebuffers.deMRT.renderPass;
			renderPassBeginInfo.framebuffer = framebuffers.deMRT.handle;

			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineManager.Get(dePipeline::MRT));

			vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport sceneViewport =  m_window.Viewport();
			vkCmdSetViewport(cmdBuffer, 0, 1, &sceneViewport);

			VkRect2D sceneScissor = m_window.Scissor();
			vkCmdSetScissor(cmdBuffer, 0, 1, &sceneScissor);

			// Binding 0 = uniform buffer
			auto& mrtUniformDescriptor = uniformBindingDescriptors[dePipeline::MRT];

			std::array<VkDescriptorBufferBindingInfoEXT, 2> descriptor_buffer_binding_info = {};
			descriptor_buffer_binding_info[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
			descriptor_buffer_binding_info[0].address =
				mrtUniformDescriptor.GetBuffer().GetDeviceAddress();
			descriptor_buffer_binding_info[0].usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;

			// Binding 1 = Image
			descriptor_buffer_binding_info[1].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
			descriptor_buffer_binding_info[1].address =
				textureSamplerDescriptor.GetBuffer().GetDeviceAddress();
			descriptor_buffer_binding_info[1].usage = VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT |
				VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;

			g_vkCmdBindDescriptorBuffersEXT(cmdBuffer, static_cast<uint32_t>(descriptor_buffer_binding_info.size()),
				descriptor_buffer_binding_info.data());

			uint32_t buffer_index_ubo = 0;
			VkDeviceSize buffer_offset = 0;

			//global transform
			g_vkCmdSetDescriptorBufferOffsetsEXT(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineLayouts[dePipeline::MRT], 0, 1, &buffer_index_ubo, &buffer_offset);

			vk::DrawInfo drawInfo = {};
			drawInfo.cmdBuffer = cmdBuffer;
			drawInfo.imageBufferIndex = 1;
			drawInfo.firstSet = 1;
			drawInfo.pipelineLayout = pipelineLayouts[dePipeline::MRT];
			drawInfo.textureBindingSize = textureSamplerDescriptor.GetLayoutSize();

			m_assetManager->DrawObjects(drawInfo);

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
			renderPassBeginInfo.renderArea.extent =
				{static_cast<uint32_t>(framebuffers.deComposition.width), static_cast<uint32_t>(framebuffers.deComposition.height)};
			renderPassBeginInfo.renderPass = framebuffers.deComposition.renderPass;
			renderPassBeginInfo.framebuffer = framebuffers.deComposition.handle;

			vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport sceneViewport = windowViewport;
			vkCmdSetViewport(cmdBuffer, 0, 1, &sceneViewport);

			VkRect2D sceneScissor = m_window.Scissor();
			vkCmdSetScissor(cmdBuffer, 0, 1, &sceneScissor);

			// Binding 0 = image samplers
			std::array<VkDescriptorBufferBindingInfoEXT, 2> descriptor_buffer_binding_info = {};

			descriptor_buffer_binding_info[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
			descriptor_buffer_binding_info[0].address =
				compositionImageBindingDescriptor.GetBuffer().GetDeviceAddress();
			descriptor_buffer_binding_info[0].usage = VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT |
				VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;

			//Binding 1 = uniform light data
			descriptor_buffer_binding_info[1].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
			descriptor_buffer_binding_info[1].address =
				uniformBindingDescriptors[dePipeline::COMPOSITION].GetBuffer().GetDeviceAddress();
			descriptor_buffer_binding_info[1].usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;

			g_vkCmdBindDescriptorBuffersEXT(cmdBuffer, static_cast<uint32_t>(descriptor_buffer_binding_info.size()),
				descriptor_buffer_binding_info.data());

			uint32_t buffer_index_images = 0;
			uint32_t buffer_index_ubo = 1;

			//image sampler set 0;
			VkDeviceSize buffer_offset = currentFrame * compositionImageBindingDescriptor.GetLayoutSize();

			g_vkCmdSetDescriptorBufferOffsetsEXT(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineLayouts[dePipeline::COMPOSITION], 0, 1, &buffer_index_images, &buffer_offset);

			//uniform set 1;
			buffer_offset = currentFrame * uniformBindingDescriptors[dePipeline::COMPOSITION].GetLayoutSize();

			g_vkCmdSetDescriptorBufferOffsetsEXT(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineLayouts[dePipeline::COMPOSITION], 1, 1, &buffer_index_ubo, &buffer_offset);

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
			renderPassBeginInfo.renderArea.extent =
				{static_cast<uint32_t>(framebuffers.deSky.width), static_cast<uint32_t>(framebuffers.deSky.height)};
			renderPassBeginInfo.renderPass = framebuffers.deSky.renderPass;
			renderPassBeginInfo.framebuffer = framebuffers.deSky.handle;

			vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdSetViewport(cmdBuffer, 0, 1, &windowViewport);

			VkRect2D sceneScissor = m_window.Scissor();
			vkCmdSetScissor(cmdBuffer, 0, 1, &sceneScissor);

			std::array<VkDescriptorBufferBindingInfoEXT, 2> descriptor_buffer_binding_info = {};
			// Binding/Set 0 = uniform data (same as MRTs)
			descriptor_buffer_binding_info[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
			descriptor_buffer_binding_info[0].address =
				uniformBindingDescriptors[dePipeline::MRT].GetBuffer().GetDeviceAddress();
			descriptor_buffer_binding_info[0].usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;

			//Binding/Set 1 = cube sampler data
			descriptor_buffer_binding_info[1].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
			descriptor_buffer_binding_info[1].address =
				skyboxSamplerBindingDescriptor.GetBuffer().GetDeviceAddress();
			descriptor_buffer_binding_info[1].usage = VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT |
				VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;

			g_vkCmdBindDescriptorBuffersEXT(cmdBuffer, static_cast<uint32_t>(descriptor_buffer_binding_info.size()),
				descriptor_buffer_binding_info.data());

			uint32_t buffer_index_ubo = 0;
			uint32_t buffer_index_sampler = 1;

			VkDeviceSize buffer_offset = currentFrame * uniformBindingDescriptors[dePipeline::MRT].GetLayoutSize();

			//global transform
			g_vkCmdSetDescriptorBufferOffsetsEXT(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineLayouts[dePipeline::SKY], 0, 1, &buffer_index_ubo, &buffer_offset);

			buffer_offset = 0;

			//scene sampler
			g_vkCmdSetDescriptorBufferOffsetsEXT(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineLayouts[dePipeline::SKY],1, 1, &buffer_index_sampler, &buffer_offset);

			vkCmdDraw(cmdBuffer, 3, 1, 0, 0);

			vkCmdEndRenderPass(cmdBuffer);
		}

		//Swapchain Quad
		{
			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineManager.Get(dePipeline::SWAPCHAIN));

			VkViewport windowViewport = m_window.Viewport();

			clearValues[0].color = { 0,0,0,0 };

			VkRenderPassBeginInfo renderPassBeginInfo = vk::init::RenderPassBeginInfo();
			renderPassBeginInfo.clearValueCount = 1;
			renderPassBeginInfo.pClearValues = clearValues;
			renderPassBeginInfo.renderArea.extent =
				{
					static_cast<uint32_t>(swapChain.framebuffers[currentImageIndex].width),
					static_cast<uint32_t>(swapChain.framebuffers[currentImageIndex].height)
				};
			renderPassBeginInfo.renderPass = swapChain.renderPass;
			renderPassBeginInfo.framebuffer = swapChain.framebuffers[currentImageIndex].handle;


			vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport sceneViewport = windowViewport;
			vkCmdSetViewport(cmdBuffer, 0, 1, &sceneViewport);

			VkRect2D sceneScissor = m_window.Scissor();
			vkCmdSetScissor(cmdBuffer, 0, 1, &sceneScissor);

			// Binding 0 = image samplers
			std::array<VkDescriptorBufferBindingInfoEXT, 1> descriptor_buffer_binding_info = {};
			descriptor_buffer_binding_info[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
			descriptor_buffer_binding_info[0].address =
				swapChainSamplerBindingDescriptor.GetBuffer().GetDeviceAddress();
			descriptor_buffer_binding_info[0].usage = VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT |
				VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;

			g_vkCmdBindDescriptorBuffersEXT(cmdBuffer, static_cast<uint32_t>(descriptor_buffer_binding_info.size()),
				descriptor_buffer_binding_info.data());

			uint32_t buffer_index_image = 0;

			VkDeviceSize buffer_offset = currentFrame * swapChainSamplerBindingDescriptor.GetLayoutSize();

			g_vkCmdSetDescriptorBufferOffsetsEXT(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineLayouts[dePipeline::SWAPCHAIN], 0, 1, &buffer_index_image, &buffer_offset);

			vkCmdDraw(cmdBuffer, 3, 1, 0, 0);

			if (m_settings.UIDisplay)
			{
				UIOverlay.Render(cmdBuffer);
			}

			vkCmdEndRenderPass(cmdBuffer);
		}

		VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));

	}
}
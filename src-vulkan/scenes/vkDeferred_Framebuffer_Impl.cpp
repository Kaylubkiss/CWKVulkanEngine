#include "vkDeferredShadingContext.h"

namespace vk
{
	void DeferredContext::InitializeFramebuffers()
	{
		InitializeDeferredFramebuffer();
		InitializeDeferredShadowFramebuffer();
		InitializeDeferredCompositionFramebuffer();
		InitializeDeferredSkyboxFramebuffer();
	}

    void DeferredContext::InitializeDeferredFramebuffer()
	{
		VkViewport viewport = m_window.Viewport();

		for (auto& MRTFramebuffer : framebuffers.deMRT)
		{
			MRTFramebuffer.Destroy();
			MRTFramebuffer.Init(&this->device);

			MRTFramebuffer.width = static_cast<uint32_t>(viewport.width);
			MRTFramebuffer.height = static_cast<uint32_t>(viewport.height);

			VkFramebufferCreateInfo framebufferCI = vk::init::FramebufferCreateInfo();
			framebufferCI.width = MRTFramebuffer.width;
			framebufferCI.height = MRTFramebuffer.height;
			framebufferCI.layers = 1;

			vk::FramebufferAttachmentCreateInfo attachmentCI = {};

			//position attachment
			attachmentCI.format = VK_FORMAT_R16G16B16A16_SFLOAT;
			attachmentCI.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			attachmentCI.width = framebufferCI.width;
			attachmentCI.height = framebufferCI.height;
			attachmentCI.layerCount = 1;
			attachmentCI.sampleCount = VK_SAMPLE_COUNT_1_BIT;
			attachmentCI.loadOP = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachmentCI.storeOP = VK_ATTACHMENT_STORE_OP_STORE;
			attachmentCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachmentCI.operatingLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			attachmentCI.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			MRTFramebuffer.AddAttachment(attachmentCI);

			//normal attachment
			MRTFramebuffer.AddAttachment(attachmentCI);

			//albedo attachment
			attachmentCI.format = VK_FORMAT_R8G8B8A8_UNORM;
			MRTFramebuffer.AddAttachment(attachmentCI);

			//metallic roughness attachment
			//because there's no optimal tiling feature for a smaller format, I'm sticking with the albedo's format for now.
			MRTFramebuffer.AddAttachment(attachmentCI);

			//ambient occlusion attachment
			MRTFramebuffer.AddAttachment(attachmentCI);

			//depth attachment
			attachmentCI.format = VK_FORMAT_D24_UNORM_S8_UINT;
			attachmentCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			attachmentCI.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			attachmentCI.operatingLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			MRTFramebuffer.AddAttachment(attachmentCI);

			MRTFramebuffer.CreateSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR,
				VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

			MRTFramebuffer.CreateFramebuffer();
		}
	}

	void DeferredContext::InitializeDeferredShadowFramebuffer()
    {
    	VkViewport viewport = m_window.Viewport();

		for (auto& shadowMapFramebuffer : framebuffers.deShadow)
		{
			shadowMapFramebuffer.Destroy();
			shadowMapFramebuffer.Init(&this->device);

			shadowMapFramebuffer.width = 2048;
			shadowMapFramebuffer.height = 2048;

			vk::FramebufferAttachmentCreateInfo attachmentCI = {};
			attachmentCI.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
			attachmentCI.width = shadowMapFramebuffer.width;
			attachmentCI.height = shadowMapFramebuffer.height;
			attachmentCI.layerCount = LIGHT_COUNT;
			attachmentCI.sampleCount = VK_SAMPLE_COUNT_1_BIT;
			attachmentCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			attachmentCI.loadOP = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachmentCI.storeOP = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachmentCI.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			attachmentCI.operatingLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			shadowMapFramebuffer.AddAttachment(attachmentCI);

			shadowMapFramebuffer.CreateSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR,
				VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

			shadowMapFramebuffer.CreateFramebuffer();
		}
    }

	void DeferredContext::InitializeDeferredCompositionFramebuffer()
	{
		VkViewport viewport = m_window.Viewport();

		for (auto& compositionFramebuffer :  framebuffers.deComposition)
		{
			compositionFramebuffer.Destroy();
			compositionFramebuffer.Init(&this->device);

			compositionFramebuffer.width = static_cast<uint32_t>(viewport.width);
			compositionFramebuffer.height = static_cast<uint32_t>(viewport.height);

			vk::FramebufferAttachmentCreateInfo attachmentCI = {};
			attachmentCI.layerCount = 1;
			attachmentCI.sampleCount = VK_SAMPLE_COUNT_1_BIT;
			attachmentCI.width = compositionFramebuffer.width;
			attachmentCI.height = compositionFramebuffer.height;
			attachmentCI.format = VK_FORMAT_B8G8R8A8_UNORM;
			attachmentCI.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			attachmentCI.loadOP = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachmentCI.storeOP = VK_ATTACHMENT_STORE_OP_STORE;
			attachmentCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachmentCI.operatingLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			attachmentCI.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			compositionFramebuffer.AddAttachment(attachmentCI);

			compositionFramebuffer.CreateSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR,
				VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

			compositionFramebuffer.CreateFramebuffer();
		}
	}

	void DeferredContext::InitializeDeferredSkyboxFramebuffer()
	{
		VkViewport viewport = m_window.Viewport();

		size_t frame = 0;

		for (auto& skyFramebuffer : framebuffers.deSky)
		{
			skyFramebuffer.Destroy();
			skyFramebuffer.Init(&this->device);

			skyFramebuffer.width = static_cast<uint32_t>(viewport.width);
			skyFramebuffer.height = static_cast<uint32_t>(viewport.height);

			auto& compositionFramebuffer = framebuffers.deComposition[frame];

			vk::FramebufferAttachmentCreateInfo attachmentCI = {};
			attachmentCI.layerCount = 1;
			attachmentCI.sampleCount = VK_SAMPLE_COUNT_1_BIT;
			attachmentCI.width = skyFramebuffer.width;
			attachmentCI.height = skyFramebuffer.height;
			attachmentCI.format = compositionFramebuffer.attachments.front().format;
			//since this is the final pass, the color of this output will be sampled by the swapchain quad.
			attachmentCI.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			attachmentCI.loadOP = VK_ATTACHMENT_LOAD_OP_LOAD;
			attachmentCI.storeOP = VK_ATTACHMENT_STORE_OP_STORE;
			attachmentCI.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			attachmentCI.operatingLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			attachmentCI.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			attachmentCI.alreadyAllocatedView = compositionFramebuffer.attachments.front().imageView;

			skyFramebuffer.AddAttachment(attachmentCI);

			//depth attachment from the mrt stage is used to compare the depth of each fragment --> more efficient
			//coloring of the frame by discarding the pixels below the threshold (1.f).

			auto& MRTFramebuffer = framebuffers.deMRT[frame];

			attachmentCI.format = MRTFramebuffer.attachments.back().format;
			attachmentCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			attachmentCI.loadOP = VK_ATTACHMENT_LOAD_OP_LOAD; //loading in the depth buffer from MRT stage.
			attachmentCI.storeOP = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachmentCI.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			attachmentCI.operatingLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			attachmentCI.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			attachmentCI.alreadyAllocatedView = MRTFramebuffer.attachments.back().imageView;

			skyFramebuffer.AddAttachment(attachmentCI);

			skyFramebuffer.CreateSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR,
				VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

			skyFramebuffer.CreateFramebuffer();

			++frame;
		}
	}

}



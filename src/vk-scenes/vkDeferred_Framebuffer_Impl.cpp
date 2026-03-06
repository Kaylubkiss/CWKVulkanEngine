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

		framebuffers.deMRT.Destroy();
		framebuffers.deMRT.Init(&this->device);

		framebuffers.deMRT.width = static_cast<uint32_t>(viewport.width);
		framebuffers.deMRT.height = static_cast<uint32_t>(viewport.height);

		VkFramebufferCreateInfo framebufferCI = vk::init::FramebufferCreateInfo();
		framebufferCI.width = framebuffers.deMRT.width;
		framebufferCI.height = framebuffers.deMRT.height;
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
		framebuffers.deMRT.AddAttachment(attachmentCI);

		//normal attachment
		framebuffers.deMRT.AddAttachment(attachmentCI);

		//albedo attachment
		attachmentCI.format = VK_FORMAT_R8G8B8A8_UNORM;
		framebuffers.deMRT.AddAttachment(attachmentCI);

		//metallic roughness attachment
		//because there's no optimal tiling feature for a smaller format, I'm sticking with the albedo's format for now.
		framebuffers.deMRT.AddAttachment(attachmentCI);

		//ambient occlusion attachment
		framebuffers.deMRT.AddAttachment(attachmentCI);

		//depth attachment
		attachmentCI.format = VK_FORMAT_D24_UNORM_S8_UINT;
		attachmentCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		attachmentCI.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		attachmentCI.operatingLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		framebuffers.deMRT.AddAttachment(attachmentCI);

		framebuffers.deMRT.CreateSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

		framebuffers.deMRT.CreateRenderPass();

		framebuffers.deMRT.CreateFramebuffer();
	}

	void DeferredContext::InitializeDeferredShadowFramebuffer()
    {
    	VkViewport viewport = m_window.Viewport();

    	framebuffers.deShadow.Destroy();
    	framebuffers.deShadow.Init(&this->device);

    	framebuffers.deShadow.width = 2048;
    	framebuffers.deShadow.height = 2048;

    	vk::FramebufferAttachmentCreateInfo attachmentCI = {};
    	attachmentCI.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
    	attachmentCI.width = framebuffers.deShadow.width;
    	attachmentCI.height = framebuffers.deShadow.height;
    	attachmentCI.layerCount = LIGHT_COUNT;
		attachmentCI.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    	attachmentCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    	attachmentCI.loadOP = VK_ATTACHMENT_LOAD_OP_CLEAR;
    	attachmentCI.storeOP = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    	attachmentCI.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    	attachmentCI.operatingLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    	framebuffers.deShadow.AddAttachment(attachmentCI);

    	framebuffers.deShadow.CreateSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

    	framebuffers.deShadow.CreateRenderPass();
    	framebuffers.deShadow.CreateFramebuffer();
    }

	void DeferredContext::InitializeDeferredCompositionFramebuffer()
	{
		VkViewport viewport = m_window.Viewport();

		framebuffers.deComposition.Destroy();
		framebuffers.deComposition.Init(&this->device);

		framebuffers.deComposition.width = static_cast<uint32_t>(viewport.width);
		framebuffers.deComposition.height = static_cast<uint32_t>(viewport.height);

		vk::FramebufferAttachmentCreateInfo attachmentCI = {};
		attachmentCI.layerCount = 1;
		attachmentCI.sampleCount = VK_SAMPLE_COUNT_1_BIT;
		attachmentCI.width = framebuffers.deComposition.width;
		attachmentCI.height = framebuffers.deComposition.height;
		attachmentCI.format = VK_FORMAT_B8G8R8A8_UNORM;
		attachmentCI.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		attachmentCI.loadOP = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentCI.storeOP = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentCI.operatingLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachmentCI.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		framebuffers.deComposition.AddAttachment(attachmentCI);

		framebuffers.deComposition.CreateSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

		framebuffers.deComposition.CreateRenderPass();

		framebuffers.deComposition.CreateFramebuffer();
	}

	void DeferredContext::InitializeDeferredSkyboxFramebuffer()
	{
		VkViewport viewport = m_window.Viewport();

		framebuffers.deSky.Destroy();
		framebuffers.deSky.Init(&this->device);

		framebuffers.deSky.width = static_cast<uint32_t>(viewport.width);
		framebuffers.deSky.height = static_cast<uint32_t>(viewport.height);

		vk::FramebufferAttachmentCreateInfo attachmentCI = {};
		attachmentCI.layerCount = 1;
		attachmentCI.sampleCount = VK_SAMPLE_COUNT_1_BIT;
		attachmentCI.width = framebuffers.deSky.width;
		attachmentCI.height = framebuffers.deSky.height;
		attachmentCI.format = framebuffers.deComposition.attachments.front().format;
		//since this is the final pass, the color of this output will be sampled by the swapchain quad.
		attachmentCI.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		attachmentCI.loadOP = VK_ATTACHMENT_LOAD_OP_LOAD;
		attachmentCI.storeOP = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentCI.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachmentCI.operatingLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachmentCI.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		attachmentCI.alreadyAllocatedView = framebuffers.deComposition.attachments.front().imageView;

		framebuffers.deSky.AddAttachment(attachmentCI);

		attachmentCI.format = framebuffers.deMRT.attachments.back().format;
		attachmentCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		attachmentCI.loadOP = VK_ATTACHMENT_LOAD_OP_LOAD; //loading in the depth buffer from MRT stage.
		attachmentCI.storeOP = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentCI.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		attachmentCI.operatingLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		attachmentCI.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		attachmentCI.alreadyAllocatedView = framebuffers.deMRT.attachments.back().imageView;

		framebuffers.deSky.AddAttachment(attachmentCI);

		framebuffers.deSky.CreateSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

		framebuffers.deSky.CreateRenderPass();

		framebuffers.deSky.CreateFramebuffer();
	}

}



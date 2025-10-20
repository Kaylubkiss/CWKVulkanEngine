#include "vkFramebuffer.h"

namespace vk 
{
	void FramebufferAttachment::Destroy(VkDevice l_device)
	{
		vkDestroyImageView(l_device, this->imageView, nullptr);
		this->imageView = VK_NULL_HANDLE;
		vkDestroyImage(l_device, this->image, nullptr);
		this->image = VK_NULL_HANDLE;
		vkFreeMemory(l_device, this->imageMemory, nullptr);
		this->imageMemory = VK_NULL_HANDLE;
	}


	inline bool FormatHasDepth(VkFormat format) 
	{
		std::array<VkFormat, 5> formats = {
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_D32_SFLOAT,
			VK_FORMAT_D24_UNORM_S8_UINT,
			VK_FORMAT_D16_UNORM_S8_UINT,
			VK_FORMAT_D16_UNORM
		};

		return std::find(formats.begin(), formats.end(), format) != formats.end();
	}

	inline bool FormatHasStencil(VkFormat format)
	{
		std::array<VkFormat, 4> formats = {
			VK_FORMAT_D24_UNORM_S8_UINT,
			VK_FORMAT_D16_UNORM_S8_UINT,
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_S8_UINT
		};

		return std::find(formats.begin(), formats.end(), format) != formats.end();

	}


	void Framebuffer::Init(vk::Device* contextDevice)
	{
		assert(contextDevice != nullptr);

		this->contextDevice = contextDevice;
	}

	void Framebuffer::Destroy() 
	{
		for (auto& attachment : attachments) 
		{
			attachment.Destroy(contextDevice->logical);
		}

		if (sampler) 
		{
			vkDestroySampler(contextDevice->logical, sampler, nullptr);
			sampler = VK_NULL_HANDLE;
		}

		if (renderPass) 
		{
			vkDestroyRenderPass(contextDevice->logical, renderPass, nullptr);
			renderPass = VK_NULL_HANDLE;
		}

		if (handle) 
		{
			vkDestroyFramebuffer(contextDevice->logical, handle, nullptr);
			handle = VK_NULL_HANDLE;
		}
	}

	void Framebuffer::CreateSampler(VkFilter minFilter, VkFilter magFilter, VkSamplerAddressMode addressMode) 
	{
		VkSamplerCreateInfo samplerCI = vk::init::SamplerCreateInfo();
		samplerCI.magFilter = magFilter;
		samplerCI.minFilter = minFilter;
		samplerCI.mipLodBias = 0.f;
		samplerCI.minLod = 0.0f;
		samplerCI.maxLod = 1.0f;
		samplerCI.addressModeU = addressMode;
		samplerCI.addressModeV = samplerCI.addressModeU;
		samplerCI.addressModeW = samplerCI.addressModeU;
		samplerCI.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

		VK_CHECK_RESULT(vkCreateSampler(contextDevice->logical, &samplerCI, nullptr, &sampler));
	}

	void Framebuffer::CreateRenderPass() 
	{
		VkRenderPassCreateInfo createInfo = vk::init::RenderPassCreateInfo();

		std::vector<VkAttachmentDescription> attachmentDesc(attachments.size());
		

		for (int i = 0; i < attachmentDesc.size(); ++i)
		{
			attachmentDesc[i].samples = VK_SAMPLE_COUNT_1_BIT;
			attachmentDesc[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachmentDesc[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachmentDesc[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachmentDesc[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachmentDesc[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			if (i == 3)
			{
				attachmentDesc[i].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; //the depth won't be read from in the composition pass
			}
			else
			{
				attachmentDesc[i].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; //cause this will be read by the contextbase's renderpass.
			}
		}

		attachmentDesc[0].format = deferredPass.position.format;
		attachmentDesc[1].format = deferredPass.normal.format;
		attachmentDesc[2].format = deferredPass.albedo.format;
		attachmentDesc[3].format = deferredPass.depth.format;

		std::array<VkAttachmentReference, 3> colorReferences;
		colorReferences[0] = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }; //position
		colorReferences[1] = { 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }; //normals
		colorReferences[2] = { 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }; //albedo

		VkAttachmentReference depthReference = { 3, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };


		std::array<VkSubpassDependency, 4> dependencies = {};

		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;


		dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].dstSubpass = 0;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		//all of the memory reads needs to be done. We're just going to overwrite whatever was written so don't need to "oversynchronize" 
		dependencies[1].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		dependencies[2].srcSubpass = 0;
		dependencies[2].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[2].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		//we're waiting for all reads and writes to be completed (since the l-buffer will be reading the color attachments, 
		// and these color attachments also need to be written to prior).
		dependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[2].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT; //next subpass will then take these color attachments and finally render them.

		dependencies[3].srcSubpass = 0;
		dependencies[3].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[3].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependencies[3].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[3].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[3].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		VkSubpassDescription subpass = {};
		subpass.colorAttachmentCount = colorReferences.size();
		subpass.pColorAttachments = colorReferences.data();
		subpass.pDepthStencilAttachment = &depthReference;
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		createInfo.pSubpasses = &subpass;
		createInfo.subpassCount = 1;
		createInfo.pAttachments = attachmentDesc.data();
		createInfo.attachmentCount = attachmentDesc.size();
		createInfo.pDependencies = dependencies.data();
		createInfo.dependencyCount = dependencies.size();


		VK_CHECK_RESULT(vkCreateRenderPass(contextDevice->logical, &createInfo, nullptr, &renderPass));
	}

	void Framebuffer::AddAttachment(const vk::FramebufferAttachmentCreateInfo& createInfo)
	{
		vk::FramebufferAttachment attachment = {};

		VkImageAspectFlags aspectMask = 0;

		if (createInfo.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
		{
			aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		}
		else if (createInfo.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			if (FormatHasDepth(createInfo.format))
			{
				aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
				attachment.flagBit |= VKC_ATTACHMENT_IS_DEPTH;
			}

			if (FormatHasStencil(createInfo.format))
			{
				aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
				attachment.flagBit |= VKC_ATTACHMENT_IS_STENCIL;
			}
		}
		

		assert(aspectMask > 0);

		assert
		(
			vk::util::FormatIsSupported(contextDevice->physical, createInfo.format,
				VK_IMAGE_TILING_OPTIMAL,
				VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
		);

		if (createInfo.alreadyAllocatedImage == VK_NULL_HANDLE)
		{
			VkImageCreateInfo imageCreateInfo = vk::init::ImageCreateInfo();
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
			imageCreateInfo.extent = { createInfo.width, createInfo.height, 1 };
			imageCreateInfo.mipLevels = 1;
			imageCreateInfo.arrayLayers = createInfo.layerCount;
			imageCreateInfo.samples = createInfo.sampleCount;
			imageCreateInfo.format = createInfo.format;
			imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageCreateInfo.usage = createInfo.usage;


			VK_CHECK_RESULT(vkCreateImage(contextDevice->logical, &imageCreateInfo, nullptr, &attachment.image));

			VkMemoryRequirements memRequirements;
			vkGetImageMemoryRequirements(contextDevice->logical, attachment.image, &memRequirements);

			VkMemoryAllocateInfo memAllocInfo = {};
			memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			memAllocInfo.allocationSize = memRequirements.size;
			memAllocInfo.memoryTypeIndex = contextDevice->GetMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			VK_CHECK_RESULT(vkAllocateMemory(contextDevice->logical, &memAllocInfo, nullptr, &attachment.imageMemory));
			VK_CHECK_RESULT(vkBindImageMemory(contextDevice->logical, attachment.image, attachment.imageMemory, 0));
		}
		else
		{
			attachment.image = createInfo.alreadyAllocatedImage;
			attachment.flagBit |= VKC_ATTACHMENT_IS_SWAPCHAIN_IMAGE;
		}


		VkImageViewCreateInfo viewInfo = vk::init::ImageViewCreateInfo();
		viewInfo.image = attachment.image;

		if (createInfo.layerCount == 1)
		{
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		}
		else 
		{
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		}

		viewInfo.format = createInfo.format;
		viewInfo.subresourceRange.aspectMask = aspectMask;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.layerCount = createInfo.layerCount;

		VK_CHECK_RESULT(vkCreateImageView(contextDevice->logical, &viewInfo, nullptr, &attachment.imageView));

		//initializing some other information...
		attachment.subresourceRange = viewInfo.subresourceRange;
		attachment.format = createInfo.format;

		//initializing the description...
		attachment.description = {};
		attachment.description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachment.description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		if (createInfo.usage & VK_IMAGE_USAGE_SAMPLED_BIT) 
		{
			attachment.description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		}
		else 
		{
			attachment.description.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		}
		attachment.description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment.description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment.description.format = createInfo.format;
		attachment.description.samples = createInfo.sampleCount;
		
		if (attachment.flagBit & VKC_ATTACHMENT_IS_DEPTH_STENCIL) 
		{
			attachment.description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		}
		else 
		{
			attachment.description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}


		this->attachments.push_back(attachment);
	}


}
#include "vkFramebuffer.h"
#include "vkInit.h"

namespace vk 
{
	void FramebufferAttachment::Destroy(VkDevice l_device)
	{
		if ((flags & VKC_ATTACHMENT_IMAGE_VIEW_IS_SHARED) == 0)
		{
			vkDestroyImageView(l_device, this->imageView, nullptr);
		}

		this->imageView = VK_NULL_HANDLE;

		if ((flags & VKC_ATTACHMENT_IMAGE_IS_SHARED) == 0)
		{
			vkFreeMemory(l_device, this->imageMemory, nullptr);
			vkDestroyImage(l_device, this->image, nullptr);
		}

		this->image = VK_NULL_HANDLE;
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

		return std::ranges::find(formats, format) != formats.end();
	}

	inline bool FormatHasStencil(VkFormat format)
	{
		std::array<VkFormat, 4> formats = {
			VK_FORMAT_D24_UNORM_S8_UINT,
			VK_FORMAT_D16_UNORM_S8_UINT,
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_S8_UINT
		};

		return std::ranges::find(formats.begin(), formats.end(), format) != formats.end();

	}

	Framebuffer::Framebuffer( vk::Device* contextDevice )
	{
		assert(contextDevice != nullptr);
		this->contextDevice = contextDevice;
	}

	Framebuffer::Framebuffer( Framebuffer&& other ) noexcept
	{
		this->handle = other.handle;
		this->attachments = std::move(other.attachments);
		this->width = other.width;
		this->height = other.height;
		this->renderPass = other.renderPass;
		this->sampler = other.sampler;
		this->contextDevice = other.contextDevice;

		other.contextDevice = nullptr;
	}

	Framebuffer& Framebuffer::operator=( Framebuffer&& other ) noexcept
	{
		if (this != &other)
		{
			std::swap(this->handle, other.handle);
			std::swap(this->attachments, other.attachments);
			std::swap(this->width, other.width);
			std::swap(this->height, other.height);
			std::swap(this->renderPass, other.renderPass);
			std::swap(this->sampler, other.sampler);
			std::swap(this->contextDevice, other.contextDevice);
		}

		return *this;
	}

	Framebuffer::~Framebuffer()
	{
		if (contextDevice != nullptr)
		{
			for (auto& attachment : attachments)
			{
				attachment.Destroy(contextDevice->GetDevice());
			}

			attachments.clear();

			if (sampler)
			{
				vkDestroySampler(contextDevice->GetDevice(), sampler, nullptr);
				sampler = VK_NULL_HANDLE;
			}

			if (renderPass)
			{
				vkDestroyRenderPass(contextDevice->GetDevice(), renderPass, nullptr);
				renderPass = VK_NULL_HANDLE;
			}

			if (handle)
			{
				vkDestroyFramebuffer(contextDevice->GetDevice(), handle, nullptr);
				handle = VK_NULL_HANDLE;
			}
		}
	}

	void Framebuffer::CreateSampler( VkFilter minFilter, VkFilter magFilter, VkSamplerAddressMode addressMode )
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

		VK_CHECK_RESULT(vkCreateSampler(contextDevice->GetDevice(), &samplerCI, nullptr, &sampler));
	}

	void Framebuffer::CreateRenderPass() 
	{
		//can add a renderpass from external source
		if (renderPass != VK_NULL_HANDLE)
		{
			return;
		}

		VkRenderPassCreateInfo createInfo = vk::init::RenderPassCreateInfo();

		std::vector<VkAttachmentReference> colorReferences;
		VkAttachmentReference depthReference = {};
		depthReference.layout = VK_IMAGE_LAYOUT_UNDEFINED;

		std::vector<VkAttachmentDescription> attachmentDescriptions;

		for (size_t i = 0; i < attachments.size(); ++i) 
		{
			if (attachments[i].flags & VKC_ATTACHMENT_IS_DEPTH_STENCIL)
			{
				if (depthReference.layout != VK_IMAGE_LAYOUT_UNDEFINED)
				{
					throw std::runtime_error("more than 1 depth attachment in framebuffer\n");
				}

				depthReference.attachment = static_cast<uint32_t>(i);
				depthReference.layout = attachments[i].layout;

			}
			else if (attachments[i].flags & VKC_ATTACHMENT_IS_COLOR)
			{
				VkAttachmentReference colorReference = {};
				colorReference.attachment = static_cast<uint32_t>(i);
				colorReference.layout = attachments[i].layout;

				colorReferences.push_back(colorReference);
			}

			attachmentDescriptions.push_back(attachments[i].description);
		}

		//initializing synchronization with subpass dependencies
		std::array<VkSubpassDependency, 4> dependencies = {};

		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

		dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].dstSubpass = 0;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

		dependencies[2].srcSubpass = 0;
		dependencies[2].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[2].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[2].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		dependencies[3].srcSubpass = 0;
		dependencies[3].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[3].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependencies[3].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependencies[3].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[3].dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

		//initialize the subpass
		VkSubpassDescription subpass = {};
		subpass.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
		subpass.pColorAttachments = colorReferences.data();

		if (depthReference.layout != VK_IMAGE_LAYOUT_UNDEFINED) 
		{
			subpass.pDepthStencilAttachment = &depthReference;
		}

		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		createInfo.pSubpasses = &subpass;
		createInfo.subpassCount = 1;
		createInfo.pAttachments = attachmentDescriptions.data();
		createInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
		createInfo.pDependencies = dependencies.data();
		createInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());


		VK_CHECK_RESULT(vkCreateRenderPass(contextDevice->GetDevice(), &createInfo, nullptr, &renderPass));
	}

	void Framebuffer::CreateFramebuffer() 
	{
		CreateRenderPass();

		VkFramebufferCreateInfo framebufferCI = vk::init::FramebufferCreateInfo();
		framebufferCI.width = width;
		framebufferCI.height = height;
		framebufferCI.layers = 1;

		std::vector<VkImageView> imageViews(attachments.size());

		for (size_t i = 0; i < imageViews.size(); ++i) 
		{
			imageViews[i] = attachments[i].imageView;
		}

		framebufferCI.attachmentCount = static_cast<uint32_t>(imageViews.size());
		framebufferCI.pAttachments = imageViews.data();
		framebufferCI.renderPass = renderPass;
		
		for (auto& attachment : attachments) 
		{
			framebufferCI.layers = std::max(framebufferCI.layers, attachment.subresourceRange.layerCount);
		}

		VK_CHECK_RESULT(vkCreateFramebuffer(contextDevice->GetDevice(), &framebufferCI, nullptr, &handle));
	}

	void Framebuffer::AddAttachment( const vk::FramebufferAttachmentCreateInfo& createInfo )
	{
		vk::FramebufferAttachment attachment = {};

		VkImageAspectFlags aspectMask = 0;

		if (createInfo.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
		{
			aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			attachment.flags = VKC_ATTACHMENT_IS_COLOR;

		}
		else if (createInfo.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			if (FormatHasDepth(createInfo.format))
			{
				aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				attachment.flags = VKC_ATTACHMENT_IS_DEPTH;
			}

			if (FormatHasStencil(createInfo.format))
			{
				aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
				attachment.flags |= VKC_ATTACHMENT_IS_STENCIL;
			}
		}
		

		assert(aspectMask > 0);
		assert(attachment.flags > 0);

		assert
		(
			vk::util::FormatIsSupported(contextDevice->GetGPU(), createInfo.format,
				VK_IMAGE_TILING_OPTIMAL,
				VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT)
		);

		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = (aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT) ? VK_IMAGE_ASPECT_DEPTH_BIT : aspectMask;
		subresourceRange.levelCount = 1;
		subresourceRange.layerCount = createInfo.layerCount;

		if (createInfo.alreadyAllocatedView == VK_NULL_HANDLE)
		{
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

				VK_CHECK_RESULT(vkCreateImage(contextDevice->GetDevice(), &imageCreateInfo, nullptr, &attachment.image));

				VkMemoryRequirements memRequirements;
				vkGetImageMemoryRequirements(contextDevice->GetDevice(), attachment.image, &memRequirements);

				VkMemoryAllocateInfo memAllocInfo = {};
				memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				memAllocInfo.allocationSize = memRequirements.size;
				memAllocInfo.memoryTypeIndex = contextDevice->GetMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

				VK_CHECK_RESULT(vkAllocateMemory(contextDevice->GetDevice(), &memAllocInfo, nullptr, &attachment.imageMemory));
				VK_CHECK_RESULT(vkBindImageMemory(contextDevice->GetDevice(), attachment.image, attachment.imageMemory, 0));
			}
			else
			{
				attachment.image = createInfo.alreadyAllocatedImage;
				attachment.flags |= VKC_ATTACHMENT_IMAGE_IS_SHARED;
			}

			VkImageViewCreateInfo viewInfo = vk::init::ImageViewCreateInfo();
			viewInfo.image = attachment.image;
			viewInfo.viewType = createInfo.layerCount > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;

			viewInfo.format = createInfo.format;
			viewInfo.components =
			{
				VK_COMPONENT_SWIZZLE_R,
				VK_COMPONENT_SWIZZLE_G,
				VK_COMPONENT_SWIZZLE_B,
				VK_COMPONENT_SWIZZLE_A
			};

			viewInfo.subresourceRange = subresourceRange;

			VK_CHECK_RESULT(vkCreateImageView(contextDevice->GetDevice(), &viewInfo, nullptr, &attachment.imageView));
		}
		else
		{
			attachment.imageView = createInfo.alreadyAllocatedView;
			attachment.flags |= VKC_ATTACHMENT_IMAGE_VIEW_IS_SHARED;
		}


		//initializing some other information...
		attachment.format = createInfo.format;
		attachment.subresourceRange = subresourceRange;

		//initializing the description...
		attachment.description = {};
		attachment.description.initialLayout = createInfo.initialLayout;
		attachment.description.loadOp = createInfo.loadOP;
		attachment.description.storeOp = createInfo.storeOP;
		attachment.description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment.description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment.description.format = createInfo.format;
		attachment.description.samples = createInfo.sampleCount;
		attachment.description.finalLayout = createInfo.finalLayout;

		attachment.layout = createInfo.operatingLayout;

		this->attachments.push_back(attachment);
	}

	const std::vector<FramebufferAttachment>& Framebuffer::GetAttachments() const
	{
		return attachments;
	}

	void Framebuffer::SetExtent( VkExtent2D extent )
	{
		width = extent.width;
		height = extent.height;
	}

	VkExtent2D Framebuffer::GetExtent() const
	{
		return {width, height};
	}

	VkRenderPass Framebuffer::GetRenderPass() const
	{
		return renderPass;
	}

	VkSampler Framebuffer::GetSampler() const
	{
		return sampler;
	}

	VkFramebuffer Framebuffer::GetHandle() const
	{
		return handle;
	}


}
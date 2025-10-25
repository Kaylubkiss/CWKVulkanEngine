#pragma once

#include "vkDevice.h"

namespace vk 
{
	typedef enum FramebufferAttachmentFlagBits
	{
		VKC_ATTACHMENT_IS_NULL = 0,
		VKC_ATTACHMENT_IS_COLOR = 1,
		VKC_ATTACHMENT_IS_DEPTH = 2,
		VKC_ATTACHMENT_IS_STENCIL = 4,
		VKC_ATTACHMENT_IS_DEPTH_STENCIL = VKC_ATTACHMENT_IS_DEPTH | VKC_ATTACHMENT_IS_STENCIL,
		VKC_ATTACHMENT_IS_SWAPCHAIN_IMAGE = 8,
		VKC_ATTACHMENT_IS_SAMPLED = 16
	} FramebufferAttachmentFlagBits;
	typedef uint32_t FramebufferAttachmentFlags;

	//NOTE: if alreadyAllocatedImage is not null: width, height, and sampleCount are not used.
	struct FramebufferAttachmentCreateInfo 
	{
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t layerCount = 1;
		VkFormat format = VK_FORMAT_UNDEFINED;
		VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;		
		VkImageUsageFlags usage = {};	

		VkImage alreadyAllocatedImage = VK_NULL_HANDLE;
	};


	struct FramebufferAttachment
	{
		VkImage image = VK_NULL_HANDLE;
		VkDeviceMemory imageMemory = VK_NULL_HANDLE;
		VkImageView imageView = VK_NULL_HANDLE;
		VkFormat format = {};
		VkImageSubresourceRange subresourceRange = {};
		VkAttachmentDescription description = {};
		FramebufferAttachmentFlags flags = 0;

		void Destroy(VkDevice l_device);
	};


	struct Framebuffer 
	{
		public:
			uint32_t width = 0;
			uint32_t height = 0;
			VkFramebuffer handle = VK_NULL_HANDLE;
			VkRenderPass renderPass = VK_NULL_HANDLE;
			VkSampler sampler = VK_NULL_HANDLE;
			std::vector<FramebufferAttachment> attachments;

		private:
			vk::Device* contextDevice = nullptr;

		public:
			void Init(vk::Device* contextDevice);
			void Destroy();
			void CreateSampler(VkFilter minFilter, VkFilter magFilter, VkSamplerAddressMode addressMode);
			void CreateRenderPass();
			void CreateFramebuffer();
			void AddAttachment(const vk::FramebufferAttachmentCreateInfo& createInfo);
	};
}

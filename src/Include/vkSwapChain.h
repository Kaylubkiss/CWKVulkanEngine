#pragma once

#include <vulkan/vulkan.h>


namespace vk 
{
	class SwapChain
	{
		private:
			VkSwapchainKHR handle = VK_NULL_HANDLE;
			uint32_t imageCount;
			
			VkImage* images;
			VkImageView* imageViews = nullptr;
			
			VkRenderPass renderPass;
			VkFramebuffer* frameBuffers = nullptr;
			
			vk::DepthResources depthInfo;

		public:
			void AllocateFrameBuffers(const VkDevice l_device, VkImageView depthImageView, const VkRect2D vpRect);


			void Destroy(VkDevice l_device)
			{
				vkDestroySwapchainKHR(l_device, this->handle, nullptr);

				for (unsigned i = 0; i < imageCount; ++i)
				{
					vkDestroyFramebuffer(l_device, this->frameBuffers[i], nullptr);
				}
			}

			SwapChain(const VkDevice l_device, const VkPhysicalDevice p_device, uint32_t graphicsFamily, uint32_t presentFamily, const VkSurfaceKHR windowSurface);

		private:
			void CreateImageViews(const VkDevice l_device, VkImage* images, uint32_t imageCount);
	};

	
}

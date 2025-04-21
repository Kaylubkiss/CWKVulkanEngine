#pragma once

#include <vulkan/vulkan.h>
#include "vkWindow.h"
#include "vkResource.h"

namespace vk 
{
	class SwapChain
	{
		private:
			VkSwapchainKHR handle = VK_NULL_HANDLE;
			uint32_t imageCount = 0;
			
			VkImage* images = nullptr;
			VkImageView* imageViews = nullptr;
			
			VkFramebuffer* frameBuffers = nullptr;

		public:
			inline SwapChain& operator=(const SwapChain& other)
			{
				if (this == &other) {
					return *this;
				}

				handle = other.handle;
				imageCount = other.imageCount;
				images = other.images;
				imageViews = other.imageViews;
				frameBuffers = other.frameBuffers;

				return *this;
			}
			SwapChain() = default;
			SwapChain(const SwapChain&) = delete;
			~SwapChain() = default;

			SwapChain(const VkDevice l_device, const VkPhysicalDevice p_device, uint32_t graphicsFamily, uint32_t presentFamily, const VkSurfaceKHR windowSurface);

			void Recreate(const VkPhysicalDevice p_device, const VkDevice l_device, uint32_t graphicsFamily, uint32_t presentFamily, vk::rsc::DepthResources& depthResources, const VkRenderPass renderPass, const vk::Window& appWindow);

			void AllocateFrameBuffers(const VkDevice l_device, const VkViewport& vp, const vk::rsc::DepthResources& depthResources, const VkRenderPass renderPass);


			void Destroy(VkDevice l_device)
			{
				for (unsigned i = 0; i < imageCount; ++i)
				{
					vkDestroyImageView(l_device, this->imageViews[i], nullptr);
					vkDestroyFramebuffer(l_device, this->frameBuffers[i], nullptr);
				}

				delete[] frameBuffers;
				delete[] images;
				delete[] imageViews;

				vkDestroySwapchainKHR(l_device, this->handle, nullptr);
			}

		

		private:
			void CreateImageViews(const VkDevice l_device, VkImage* images, uint32_t imageCount);

		friend class GraphicsSystem;
	};

	
}

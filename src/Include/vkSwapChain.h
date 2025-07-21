#pragma once

#include <vulkan/vulkan.h>
#include "vkWindow.h"
#include "vkResource.h"

namespace vk 
{
	struct SwapChain
	{
			VkSwapchainKHR handle = VK_NULL_HANDLE;
			
			std::vector<VkImage> images;
			std::vector<VkImageView> imageViews;
			
			std::vector<VkFramebuffer> frameBuffers;

			inline SwapChain& operator=(const SwapChain& other)
			{
				if (this == &other) {
					return *this;
				}

				handle = other.handle;
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

			VkFramebuffer FrameBuffer(int i) 
			{
				return this->frameBuffers[i];
			}

			void Destroy(VkDevice l_device)
			{
				for (unsigned i = 0; i < this->images.size(); ++i)
				{
					vkDestroyImageView(l_device, this->imageViews[i], nullptr);
					vkDestroyFramebuffer(l_device, this->frameBuffers[i], nullptr);
				}

				vkDestroySwapchainKHR(l_device, this->handle, nullptr);
			}

		

		private:
			void CreateImageViews(const VkDevice l_device, VkImage* images, uint32_t imageCount);

	};

	
}

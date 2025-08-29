#pragma once

#include <vulkan/vulkan.h>
#include "vkWindow.h"
#include "vkResource.h"
#include "vkDevice.h"

namespace vk 
{
	struct SwapChain
	{
		private:
			VkDevice logicalDevice = VK_NULL_HANDLE;
			VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

		public:
			VkSwapchainKHR handle = VK_NULL_HANDLE;

			VkSwapchainCreateInfoKHR createInfo = {};
			
			std::vector<VkImage> images;
			std::vector<VkImageView> imageViews;
			
			std::vector<VkFramebuffer> frameBuffers;

			SwapChain() = default;
			SwapChain(const Device* devicePtr, uint32_t graphicsFamily, uint32_t presentFamily, const VkSurfaceKHR windowSurface);
			
			void Destroy();

			void Recreate(vk::rsc::DepthStencil& depthResources, const VkRenderPass renderPass, const vk::Window& appWindow);

			void AllocateFrameBuffers(const VkViewport& vp, const vk::rsc::DepthStencil& depthResources, const VkRenderPass renderPass);


		private:
			void CreateImageViews(VkImage* images, uint32_t imageCount);

	};

	
}

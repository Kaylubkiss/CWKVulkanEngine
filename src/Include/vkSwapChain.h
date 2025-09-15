#pragma once

#include <vulkan/vulkan.h>
#include "vkWindow.h"
#include "vkDevice.h"
#include "vkGlobal.h"

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
			
			FramebufferAttachment depthAttachment;
			std::vector<VkFramebuffer> frameBuffers;

			SwapChain() = default;
			SwapChain(const Device* devicePtr, const std::array<uint32_t, 2>& queueFamilies, const vk::Window& appWindow);
			
			void Destroy();

			void Recreate(const VkRenderPass renderPass, const vk::Window& appWindow);

			void CreateFrameBuffers(const VkViewport& vp, const VkRenderPass renderPass);
			


		private:
			void CreateImageViews();

	};

	
}

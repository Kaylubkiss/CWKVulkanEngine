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
			Device* devicePtr = nullptr;

		public:
			VkSwapchainKHR handle = VK_NULL_HANDLE;

			VkSwapchainCreateInfoKHR createInfo = {};
			
			std::vector<VkImage> images;
			std::vector<VkImageView> imageViews;
			
			FramebufferAttachment depthAttachment;
			std::vector<VkFramebuffer> frameBuffers;

			SwapChain() = default;
			~SwapChain() = default;

			void Init(Device* devicePtr, const vk::Window& appWindow);
			void Create(const vk::Window& appWindow);
			
			void Destroy();

			void Recreate(const VkRenderPass renderPass, const vk::Window& appWindow);

			void CreateFrameBuffers(const VkViewport& vp, const VkRenderPass renderPass);
			


		private:
			void CreateImageViews();

	};

	
}

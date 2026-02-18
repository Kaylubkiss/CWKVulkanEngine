#pragma once

#include "vkFramebuffer.h"

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
			std::vector<vk::Framebuffer> framebuffers;

			SwapChain() = default;
			~SwapChain() = default;

			void Init( Device* devicePtr, const vk::Window& appWindow );
			void Create( const vk::Window& appWindow );
			
			void Destroy();

			void Recreate(const VkRenderPass renderPass, const vk::Window& appWindow);

			void CreateFrameBuffers(const VkViewport& vp, const VkRenderPass renderPass);

	};

	
}

#pragma once

#include "vkFramebuffer.h"

namespace vk 
{
	class SwapChain
	{
	public:
		VkSwapchainKHR handle = VK_NULL_HANDLE;

		VkSwapchainCreateInfoKHR createInfo = {};

		std::vector<VkImage> images;
		std::vector<vk::Framebuffer> framebuffers;

		VkRenderPass renderPass = VK_NULL_HANDLE;

		SwapChain() = default;
		~SwapChain() = default;

		void Destroy();
		void Init( Device* devicePtr, const vk::Window& appWindow );
		void Create( const vk::Window& appWindow );
		void Recreate( const vk::Window& appWindow );
	private:
		void CreateRenderPass();
		void CreateFrameBuffers( const VkViewport& vp );
	private:
		Device* m_devicePtr = nullptr;


	};

	
}

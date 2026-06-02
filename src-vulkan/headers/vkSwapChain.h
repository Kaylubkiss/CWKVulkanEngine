#pragma once

#include "vkFramebuffer.h"

namespace vk 
{
	class SwapChain
	{
	public:
		SwapChain() = default;
		SwapChain( Device* devicePtr, const vk::Window& appWindow );

		SwapChain( const SwapChain& other ) = delete;
		SwapChain& operator=( const SwapChain& other ) = delete;

		SwapChain( SwapChain&& other ) noexcept;
		SwapChain& operator=( SwapChain&& other ) noexcept;

		~SwapChain();

		void Create( const vk::Window& appWindow );
		void Recreate( const vk::Window& appWindow );

		const std::vector<vk::Framebuffer>& GetFramebuffers() const;
		const std::vector<VkImage>& GetImages() const;
		VkRenderPass GetRenderPass() const;
		VkSwapchainKHR GetHandle() const;
	private:
		void CreateRenderPass();
		void CreateFrameBuffers( const VkViewport& vp );
	private:
		std::vector<vk::Framebuffer> framebuffers;
		std::vector<VkImage> images;

		VkSwapchainKHR handle = VK_NULL_HANDLE;
		VkSwapchainCreateInfoKHR createInfo = {};
		VkRenderPass renderPass = VK_NULL_HANDLE;

		Device* m_devicePtr = nullptr;


	};

	
}

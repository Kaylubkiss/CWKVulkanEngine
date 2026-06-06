#ifndef VK_SWAPCHAIN_HPP
#define VK_SWAPCHAIN_HPP

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

		const std::vector<VkFramebuffer>& GetFramebuffers() const;
		const std::vector<VkImage>& GetImages() const;
		VkRenderPass GetRenderPass() const;
		VkSwapchainKHR GetHandle() const;
		VkExtent2D GetExtent() const;
	private:
		void CreateRenderPass();
		void CreateFrameBuffers( const VkViewport& vp );
		void DestroyFramebufferResources();
	private:
		std::vector<VkFramebuffer> framebuffers;
		std::vector<VkImage> images;
		std::vector<VkImageView> imageViews;

		VkSwapchainKHR handle = VK_NULL_HANDLE;
		VkSwapchainCreateInfoKHR createInfo = {};
		VkRenderPass renderPass = VK_NULL_HANDLE;

		VkExtent2D m_extent = {};

		Device* m_devicePtr = nullptr;


	};

	
}

#endif

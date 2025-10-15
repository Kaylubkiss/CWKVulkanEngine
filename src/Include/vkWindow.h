#pragma once

namespace vk 
{
	struct Window 
	{
		VkViewport viewport = {};
		VkRect2D scissor = {};

		SDL_Window* sdl_ptr = nullptr;
		VkSurfaceKHR surface = VK_NULL_HANDLE;

		VkPhysicalDevice contextPhysicalDevice = VK_NULL_HANDLE;

		int center_x = 0.f;
		int center_y = 0.f;

		bool isPrepared = false;
		bool isMinimized = false;

		~Window();

		void UpdateExtents(const VkExtent2D& area);
	};

}

#pragma once

#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>

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

		~Window();

		void UpdateExtents(const VkExtent2D& area);

		bool IsMinimized();
	};

}

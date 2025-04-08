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

		int center_x = 0.f;
		int center_y = 0.f;

		~Window() 
		{
			SDL_DestroyWindow(sdl_ptr);
			SDL_Quit();
		}

		void UpdateExtents(const VkExtent2D& area)
		{
			viewport.width = area.width;
			viewport.height = area.height;

			center_x = viewport.width * 0.5f;
			center_y = viewport.height * 0.5f;

			scissor.extent.width = area.width;
			scissor.extent.height = area.height;
		}
	};

}

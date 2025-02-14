#pragma once

#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>

namespace vk 
{
	struct Window 
	{
		VkViewport viewport;
		VkRect2D scissor;

		SDL_Window* sdl_ptr;
		VkSurfaceKHR surface;

		~Window() 
		{
			SDL_DestroyWindow(sdl_ptr);

			SDL_Quit();
		}

		void UpdateExtents(const VkExtent2D& area)
		{
			viewport.width = area.width;
			viewport.height = area.height;
		}
	};

}

#include "vkWindow.h"

namespace vk 
{
	Window::~Window()
	{
		SDL_DestroyWindow(sdl_ptr);
		SDL_Quit();
	}

	void Window::UpdateExtents(const VkExtent2D& area)
	{
		viewport.width = area.width;
		viewport.height = area.height;

		center_x = viewport.width * 0.5f;
		center_y = viewport.height * 0.5f;

		scissor.extent.width = area.width;
		scissor.extent.height = area.height;
	}
}
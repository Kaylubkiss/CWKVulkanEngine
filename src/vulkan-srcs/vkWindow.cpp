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
		viewport.width = static_cast<float>(area.width);
		viewport.height = static_cast<float>(area.height);

		center_x = static_cast<int>(viewport.width * 0.5f);
		center_y = static_cast<int>(viewport.height * 0.5f);

		scissor.extent.width = area.width;
		scissor.extent.height = area.height;
	}

	VkExtent2D Window::Extents() const
	{
		return { static_cast<uint32_t>(viewport.width), static_cast<uint32_t>(viewport.height) };
	}
}
#include "vkWindow.h"

namespace vk 
{

	Window::~Window()
	{
		SDL_DestroyWindow(m_sdlPtr);
	}

	void Window::Init( uint32_t width, uint32_t height )
	{
		m_viewport.width = static_cast<float>(width);
		m_viewport.height = static_cast<float>(height);
		m_viewport.minDepth = 0;
		m_viewport.maxDepth = 1;

		m_scissor.extent.width = width;
		m_scissor.extent.height = height;

		m_sdlPtr = SDL_CreateWindow("Caleb's Vulkan Engine", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_INPUT_FOCUS);

		if (m_sdlPtr == nullptr)
		{
			printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		}

		SDL_RaiseWindow(m_sdlPtr);

		isPrepared = true;
	}

	void Window::CreateSurface( VkInstance vulkanInstance )
	{
		if (m_surface == VK_NULL_HANDLE)
		{
			if (SDL_Vulkan_CreateSurface(m_sdlPtr, vulkanInstance, &m_surface) != SDL_TRUE)
			{
				throw std::runtime_error("could not create window surface! " + std::string(SDL_GetError()));
			}
		}
	}

	VkExtent2D Window::Extents() const
	{
		return { static_cast<uint32_t>(m_viewport.width), static_cast<uint32_t>(m_viewport.height) };
	}

	VkViewport Window::Viewport() const
	{
		return m_viewport;
	}

	VkRect2D Window::Scissor() const
	{
		return m_scissor;
	}

	VkSurfaceKHR Window::Surface() const
	{
		return m_surface;
	}

	SDL_Window* Window::WindowPtr() const
	{
		return m_sdlPtr;
	}

	std::vector<const char*> Window::GetInstanceExtensions() const
	{
		//must get the SDL extensions to use SDL2
		uint32_t sdl_extensionCount = 0;
		std::vector<const char*> extensionNames;

		if (SDL_Vulkan_GetInstanceExtensions(m_sdlPtr, &sdl_extensionCount, nullptr) != SDL_TRUE)
		{
			throw std::runtime_error("could not grab extensions from SDL!");
		}

		extensionNames.resize(sdl_extensionCount);

		if (SDL_Vulkan_GetInstanceExtensions(m_sdlPtr, &sdl_extensionCount, extensionNames.data()) != SDL_TRUE)
		{
			throw std::runtime_error("could not grab extensions from SDL!");
		}

		return extensionNames;
	}

	bool Window::IsPrepared() const
	{
		return isPrepared;
	}

	bool Window::IsMinimized() const
	{
		constexpr float epsilon = 0.0001f;
		return (m_viewport.width <= epsilon || m_viewport.height == epsilon);
	}

	void Window::UpdateExtents(const VkExtent2D& area)
	{
		m_viewport.width = static_cast<float>(area.width);
		m_viewport.height = static_cast<float>(area.height);

		isPrepared = !IsMinimized();

		m_scissor.extent.width = area.width;
		m_scissor.extent.height = area.height;
	}
}
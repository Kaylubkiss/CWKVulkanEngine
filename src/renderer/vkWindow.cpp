#include "vkWindow.h"

namespace vk 
{


	Window::Window( Window&& other ) noexcept
	{
		this->m_scissor = other.m_scissor;
		this->m_viewport = other.m_viewport;
		this->m_surface = other.m_surface;
		this->isPrepared = other.isPrepared;
		this->m_sdlPtr = other.m_sdlPtr;
		this->c_instance = other.c_instance;

		other.m_sdlPtr = nullptr;
	}

	Window& Window::operator=( Window&& other ) noexcept
	{
		if (this != &other)
		{
			std::swap(this->m_scissor, other.m_scissor);
			std::swap(this->m_viewport, other.m_viewport);
			std::swap(this->m_surface, other.m_surface);
			std::swap(this->isPrepared, other.isPrepared);
			std::swap(this->m_sdlPtr, other.m_sdlPtr);
			std::swap(this->c_instance, other.c_instance);
		}

		return *this;
	}

	Window::~Window()
	{
		if (m_sdlPtr != nullptr)
		{
			SDL_DestroyWindow(m_sdlPtr);

			if (c_instance != VK_NULL_HANDLE)
			{
				vkDestroySurfaceKHR(c_instance, m_surface, nullptr);
			}

			SDL_Quit();
		}
	}

	Window::Window( uint32_t width, uint32_t height )
	{

		if (SDL_Init(SDL_INIT_VIDEO) < 0)
		{
			std::cerr << "SDL could not initialize! SDL_Error " << SDL_GetError() << '\n';
			throw std::runtime_error("Could not initialize window\n");
		}

		m_viewport.width = static_cast<float>(width);
		m_viewport.height = static_cast<float>(height);
		m_viewport.minDepth = 0.f;
		m_viewport.maxDepth = 1.f;

		m_scissor.extent.width = width;
		m_scissor.extent.height = height;

		m_sdlPtr = SDL_CreateWindow("CWKVulkan-0", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
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
		if (vulkanInstance != VK_NULL_HANDLE)
		{
			c_instance = vulkanInstance;

			if (m_surface == VK_NULL_HANDLE)
			{
				if (SDL_Vulkan_CreateSurface(m_sdlPtr, vulkanInstance, &m_surface) != SDL_TRUE)
				{
					throw std::runtime_error("could not create window surface! " + std::string(SDL_GetError()));
				}
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

	VkInstance Window::GetContextInstance() const
	{
		return c_instance;
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
		return (m_viewport.width <= epsilon || m_viewport.height <= epsilon);
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
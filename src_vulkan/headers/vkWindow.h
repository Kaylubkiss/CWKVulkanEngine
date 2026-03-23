#pragma once

namespace vk 
{
	class Window
	{
	public:
		Window() = default;
		~Window();

		void Init( uint32_t width, uint32_t height );
		void CreateSurface( VkInstance vulkanInstance );

		VkExtent2D Extents() const;
		VkViewport Viewport() const;
		VkRect2D Scissor() const;
		VkSurfaceKHR Surface() const;
		SDL_Window* WindowPtr() const;
		std::vector<const char*> GetInstanceExtensions() const;

		bool IsPrepared() const;
		bool IsMinimized() const;
		void UpdateExtents(const VkExtent2D& area);
	private:
		VkViewport m_viewport = {};
		VkRect2D m_scissor = {};

		SDL_Window* m_sdlPtr = nullptr;
		VkSurfaceKHR m_surface = VK_NULL_HANDLE;

		bool isPrepared = false;
	};

}

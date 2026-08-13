#ifndef VK_WINDOW_HPP
#define VK_WINDOW_HPP

namespace vk 
{
	class Window
	{
	public:
		Window() = default;
		Window( uint32_t width, uint32_t height );


		Window( const Window& other ) = delete;
		Window& operator=( const Window& other ) = delete;

		Window( Window&& other ) noexcept;
		Window& operator=( Window&& other ) noexcept;

		~Window();

		void CreateSurface( VkInstance vulkanInstance );

		VkExtent2D Extents() const;
		VkViewport Viewport() const;
		VkRect2D Scissor() const;
		VkSurfaceKHR Surface() const;
		SDL_Window* WindowPtr() const;
		VkInstance GetContextInstance() const;
		std::vector<const char*> GetInstanceExtensions() const;

		bool IsPrepared() const;
		bool IsMinimized() const;
		void UpdateExtents(const VkExtent2D& area);
	private:
		VkViewport m_viewport = {};
		VkRect2D m_scissor = {};

		SDL_Window* m_sdlPtr = nullptr;
		VkSurfaceKHR m_surface = VK_NULL_HANDLE;

		VkInstance c_instance = VK_NULL_HANDLE;

		bool isPrepared = false;
	};

}

#endif

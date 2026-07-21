#ifndef VK_RENDERER_BASE_HPP
#define VK_RENDERER_BASE_HPP

#include "vkWindow.h"
#include "vkInstance.h"
#include "AssetManager.h"
#include "Camera.h"
#include "vkSwapChain.h"
#include "vkFramebuffer.h"
#include "vkPipelineManager.h"
#include "UserInterface.h"

namespace vk
{
	class DescriptorManager;
}

struct TextureUploadSemaphores
{
	VkSemaphore transferSubmitSemaphore = VK_NULL_HANDLE;
	VkSemaphore graphicsSubmitSemaphore = VK_NULL_HANDLE;
};

namespace vk
{
	class RendererBase
	{
	public:
		RendererBase(); /* expect this to be derived from */
		virtual ~RendererBase();

		//getters(s)
		[[nodiscard]] const vk::Device* GetDevicePtr() const;
		Camera& GetCamera();
		vk::Window& GetWindow();

		virtual void Render( SceneView sceneView ) = 0;

		//operations
		void WaitForDevice() const;
		void ToggleUIActive(bool enable);
	protected:
		bool PrepareFrame();
		void SubmitFrame();
		virtual void UpdateUI() = 0;
		virtual void ResizeWindow();
	private:
		void CreateSynchronizationPrimitives();
	protected:
		struct Settings //TODO: make this into a bitmask
		{
			uint32_t maxFramesInFlight = 2;
			bool minimized = false;
			bool UIDisplay = true;
			bool UIToggled = false; //can consume inputs upon intialization
			bool validationLayers = true;
			bool hotReloadRequested = false;
		} m_settings = {};

		float cameraFOV = 45.f;
		Camera mCamera;
		UserInterface UIOverlay;

		vk::Instance m_instance;
		vk::Window m_window;
		vk::Device device;
		vk::SwapChain swapChain;
		vk::PipelineManager pipelineManager;

		uint32_t currentFrame = 0;
		uint32_t currentImageIndex = 0;

		std::array<VkCommandBuffer, gMaxFramesInFlight> commandBuffers;
		std::array<VkSemaphore, gMaxFramesInFlight> presentCompleteSemaphores;
		std::array<VkSemaphore, gMaxFramesInFlight> renderCompleteSemaphores;
		std::array<TextureUploadSemaphores, gMaxFramesInFlight> textureUploadSemaphores; //for I/O synchronization
		std::array<VkFence, gMaxFramesInFlight> inFlightFences;

		TextureManager* m_textureManagerPtr = nullptr;
	};
}	

#endif
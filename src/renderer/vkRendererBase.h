#ifndef VK_RENDERER_BASE_HPP
#define VK_RENDERER_BASE_HPP

#include "renderer/vkWindow.h"
#include "vkInstance.h"
#include "AssetLoader.h"
#include "renderer/vkSwapChain.h"
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
		RendererBase( vk::Device& device, vk::Window& window, vk::TextureManager& textureManager ); /* expect this to be derived from */
		virtual ~RendererBase();

		virtual void Render( SceneView sceneView ) = 0;

		//operations
		void WaitForDevice() const;
	protected:
		bool PrepareFrame();
		void SubmitFrame();
		virtual void UpdateUI() = 0;
		virtual void ResizeWindow();
	private:
		void CreateSynchronizationPrimitives();
	protected:
		struct Settings
		{
			uint32_t maxFramesInFlight = 2;
			bool minimized = false;
			bool UIDisplay = true;
			bool validationLayers = true;
			bool hotReloadRequested = false;
		} m_settings = {};

		vk::Device& c_device;
		vk::Window& c_window;

		vk::SwapChain swapChain;
		vk::PipelineManager pipelineManager;

		uint32_t currentFrame = 0;
		uint32_t currentImageIndex = 0;

		std::array<VkCommandBuffer, gMaxFramesInFlight> commandBuffers;
		std::array<VkSemaphore, gMaxFramesInFlight> presentCompleteSemaphores;
		std::array<VkSemaphore, gMaxFramesInFlight> renderCompleteSemaphores;
		std::array<TextureUploadSemaphores, gMaxFramesInFlight> textureUploadSemaphores; //for I/O synchronization
		std::array<VkFence, gMaxFramesInFlight> inFlightFences;

		TextureManager& c_textureManager;
	};
}	

#endif
#pragma once

#include "vkWindow.h"
#include "vkInstance.h"
#include "AssetManager.h"
#include "vkSwapChain.h"
#include "vkFramebuffer.h"
#include "vkPipelineManager.h"
#include "UserInterface.h"

namespace vk
{
	class ContextBase
	{
	public:
		ContextBase( TextureManager* textureManagerPtr ); /* expect this to be derived from */
		virtual ~ContextBase();

		//getters(s)
		[[nodiscard]] GraphicsContextInfo& GetGraphicsContextInfo();
		Camera& GetCamera();
		vk::Window& GetWindow();

		virtual void Render( AssetManager& assetManager ) {};

		//operations
		void WaitForDevice() const;
		void ToggleUIActive(bool enable);
	protected:
		bool PrepareFrame();
		void SubmitFrame();
		virtual void RecordCommandBuffers( AssetManager& assetManager ) {};
		virtual void UpdateUI() {};
		virtual void ResizeWindow();
		virtual void InitializePipeline() {};
		virtual void InitializeDescriptors() {};
		virtual void FillOutGraphicsContextInfo();
	private:
		void CreateSynchronizationPrimitives();

	protected:
		struct AppSettings //TODO: make this into a bitmask
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

		//this is for textureManager and potentially any other discrete systems.
		GraphicsContextInfo m_info;

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
		std::array<VkSemaphore, gMaxFramesInFlight> textureUploadSemaphores; //for I/O synchronization
		std::array<VkFence, gMaxFramesInFlight> inFlightFences;

		/*VkPipelineLayout m_graphicsPipelineLayout = VK_NULL_HANDLE;*/ //TODO

		TextureManager* m_textureManagerPtr = nullptr;


	};
}	

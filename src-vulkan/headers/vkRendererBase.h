#pragma once

#include "vkWindow.h"
#include "vkInstance.h"
#include "AssetManager.h"
#include "Camera.h"
#include "vkSwapChain.h"
#include "vkFramebuffer.h"
#include "vkPipelineManager.h"
#include "UserInterface.h"


class DescriptorManager;

namespace vk
{
	class RendererBase
	{
	public:
		RendererBase(); /* expect this to be derived from */
		virtual ~RendererBase();

		//getters(s)
		[[nodiscard]] GraphicsContextInfo& GetInfo();
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
		virtual void InitializeDescriptors( DescriptorManager& descriptorManager ) {};
		virtual void FillOutGraphicsContextInfo();
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

		//this is for textureManager and potentially any other discrete systems.
		GraphicsContextInfo m_info = {};

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

		TextureManager* m_textureManagerPtr = nullptr;
	};
}	

#pragma once

#include "vkWindow.h"
#include "vkInstance.h"
#include "vkSwapChain.h"
#include "vkFramebuffer.h"
#include "vkPipelineManager.h"
#include "../../src_core/managers/SceneManager.h"
#include "UserInterface.h"

namespace vk
{
	class ContextBase
	{
	public:
		ContextBase() = default;
		virtual ~ContextBase();
		//getters(s)
		[[nodiscard]] const GraphicsContextInfo& GetGraphicsContextInfo() const;
		Camera& GetCamera();
		vk::Window& GetWindow();
		//operations
		virtual void Init();
		virtual void Render( Scene& scene ) = 0;
		virtual void WaitForDevice() const;
		virtual void ResizeWindow();
		virtual void ToggleUIActive(bool enable);
	protected:
		virtual bool PrepareFrame();
		virtual void RecordCommandBuffers( Scene& scene ) = 0;
		virtual void SubmitFrame();
		virtual void UpdateUI() {};
		virtual void InitializePipeline() = 0;
		virtual void InitializeDescriptors() = 0;
		virtual void FillOutGraphicsContextInfo();
		virtual void UploadPendingTexturesToGPU( /*std::vector<PendingTextureInfo>& pendingTextures*/ );
	private:
		void CreateSynchronizationPrimitives();
	protected:
		struct ContextSettings //TODO: make this into a bitmask
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
		//vk::DescriptorBuffer textureSamplerDescriptor;
		vk::PipelineManager pipelineManager;

		uint32_t currentFrame = 0;
		uint32_t currentImageIndex = 0;

		std::array<VkCommandBuffer, gMaxFramesInFlight> commandBuffers; //command pool is in vk::Device
		std::array<VkSemaphore, gMaxFramesInFlight> presentCompleteSemaphores;
		std::array<VkSemaphore, gMaxFramesInFlight> renderCompleteSemaphores;
		std::array<VkSemaphore, gMaxFramesInFlight> textureUploadSemaphores; //for I/O synchronization
		std::array<VkFence, gMaxFramesInFlight> inFlightFences;

		DescriptorManager* m_descriptorManagerPtr = nullptr;
	};
}	

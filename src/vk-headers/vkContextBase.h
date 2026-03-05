#pragma once

#include "vkWindow.h"
#include "vkInstance.h"
#include "ObjectManager.h"
#include "vkSwapChain.h"
#include "vkFramebuffer.h"
#include "vkPipelineManager.h"
#include "UserInterface.h"

namespace vk
{
	class ContextBase
	{
	public:
		ContextBase(); /* expect this to be derived from */
		virtual ~ContextBase();

		//getters(s)
		[[nodiscard]] std::weak_ptr<GraphicsContextInfo> GetGraphicsContextInfo() const;
		Camera& GetCamera();
		vk::Window& GetWindow();

		virtual void Render() {};
		virtual void RecordCommandBuffers() {};
		virtual void InitializeScene() {};

		//operations
		void WaitForDevice() const;
		void SubmitFrame();
		void ResizeWindow();
		void UpdateSceneObjects(float dt) const;
		void ToggleUIActive(bool enable);
	protected:
		bool PrepareFrame();
		virtual void UpdateUI() {};
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
		} m_settings = {};

		float cameraFOV = 45.f;
		Camera mCamera;
		UserInterface UIOverlay;

		//this is for textureManager and potentially any other discrete systems.
		std::shared_ptr<GraphicsContextInfo> m_info;

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

		std::shared_ptr<ObjectManager> m_objectManager;
	};
}	

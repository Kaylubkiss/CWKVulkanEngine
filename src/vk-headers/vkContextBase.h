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
		std::shared_ptr<GraphicsContextInfo> GetGraphicsContextInfo() const;
		Camera& GetCamera();
		vk::Window& GetWindow();

		//public virtual function(s)
		virtual void Render() = 0;
		virtual void RecordCommandBuffers() = 0;
		virtual void UpdateUI() = 0;
		virtual void ResizeWindowDerived() = 0;
		virtual void InitializeScene() = 0;

		//operations
		void WaitForDevice() const;
		void SubmitFrame();
		void ResizeWindow();
		void UpdateSceneObjects(float dt) const;
		void ToggleUI(bool enable);
	protected:
		bool PrepareFrame();
		//more pure virtual function(s)
		virtual void InitializePipeline( std::string vsFile, std::string fsFile ) = 0;
		virtual void InitializeDescriptors() = 0;
		//non-pure virtual functions
		virtual void InitializeRenderPass();
		virtual void FillOutGraphicsContextInfo();
	private:
		void CreateSynchronizationPrimitives();
	protected:

		struct AppSettings //TODO: make this into a bitmask
		{
			uint32_t maxFramesInFlight = 2;
			bool minimized = false;
			bool UIDisplay = true;
			bool UIToggled = true; //can consume inputs upon intialization
			bool validationLayers = true;
		} m_settings = {};

		//this is for textureManager and potentially any other discrete systems.
		std::shared_ptr<GraphicsContextInfo> m_info;

		//MUST BE DEINIT LAST!

		VkRenderPass renderPass = VK_NULL_HANDLE;

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

		float FOV = 45.f;

		Camera mCamera;
		UserInterface UIOverlay;

		std::unique_ptr<ObjectManager> m_objectManager;
	};
}	

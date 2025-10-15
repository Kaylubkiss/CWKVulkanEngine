#pragma once

#include "vkSwapChain.h"
#include "vkPipelineManager.h"
#include "vkDevice.h"
#include "UserInterface.h"
#include "Camera.h"

namespace vk
{

	/* NOTE: JANK FORWARD DECLARATION, BECAUSE OF A DOUBLE INCLUDE PROBABLY */
	class ObjectManager;

	class ContextBase
	{
		protected:

			GraphicsContextInfo mInfo;//this is for textureManager and potentially any other discrete systems.
			//WARNING: context specific!!!

			struct {
				uint32_t maxFramesInFlight = 2;
				bool minimized = false;
				const bool UIEnabled = true;
			} settings;

			vk::Window window;

			VkInstance instance = VK_NULL_HANDLE;

			vk::Device device;

			VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
			
			uint32_t currentFrame = 0;
			uint32_t currentImageIndex = 0;

			VkCommandPool commandPool = VK_NULL_HANDLE;
			std::array<VkCommandBuffer, gMaxFramesInFlight> commandBuffers;

			std::array<VkSemaphore, gMaxFramesInFlight> presentCompleteSemaphores;
			std::array<VkSemaphore, gMaxFramesInFlight> renderCompleteSemaphores;

			std::array<VkFence, gMaxFramesInFlight> inFlightFences;

			vk::SwapChain swapChain;

			vk::PipelineManager pipelineManager;

			VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
			VkRenderPass renderPass = VK_NULL_HANDLE;

			float FOV = 45.f;
			Camera mCamera;
			vk::UserInterface UIOverlay;

			bool isInitialized = false;

		public: 

			ContextBase(); /* expect this to be derived from */
			virtual ~ContextBase();

			//pure virtual function(s)
			virtual void RecordCommandBuffers() = 0;
			virtual void UpdateUI();
			virtual void ResizeWindowDerived();
			virtual void InitializeScene(ObjectManager& objManager) = 0;
			
			GraphicsContextInfo GetGraphicsContextInfo();
			
			//public virtual function(s)
			virtual void Render() = 0;
			
			//getter(s)
			const VkPhysicalDevice PhysicalDevice() const;
			const VkDevice LogicalDevice() const;

			Camera& GetCamera();
			vk::Window& GetWindow();

			//operations
			void WaitForDevice();
			void SubmitFrame();
			void ToggleRendering();
			void ResizeWindow();

		protected:

			void PrepareFrame();
			//more pure virtual function(s)
			virtual void InitializePipeline(std::string vsFile = "", std::string fsFile = "") = 0;
			virtual void InitializeDescriptors() = 0;

			//non-pure virtual functions
			virtual void InitializeRenderPass();

			virtual void FillOutGraphicsContextInfo();

		

		private:
			void CreateWindow();
			void CreateInstance();
			void CreateSynchronizationPrimitives();
			//void CreateUI();
	};
}	

#pragma once

#include "vkWindow.h"
#include "vkSwapChain.h"
#include "vkFramebuffer.h"
#include "vkPipelineManager.h"

namespace vk
{

	/* NOTE: JANK FORWARD DECLARATION, BECAUSE OF A DOUBLE INCLUDE PROBABLY */
	class ObjectManager;

	class ContextBase
	{
		protected:

			GraphicsContextInfo mInfo;//this is for textureManager and potentially any other discrete systems.
			//WARNING: context specific!!!

			struct Settings {
				uint32_t maxFramesInFlight = 2;
				bool minimized = false;
				bool UIEnabled = true;
				bool validationLayers = true;
				VkDebugUtilsMessengerEXT debugMessenger = nullptr;
			} settings;

			vk::Window window;

			VkInstance instance = VK_NULL_HANDLE;

			vk::Device device;

			VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
			
			uint32_t currentFrame = 0;
			uint32_t currentImageIndex = 0;

			std::array<VkCommandBuffer, gMaxFramesInFlight> commandBuffers;

			std::array<VkSemaphore, gMaxFramesInFlight> presentCompleteSemaphores;
			std::array<VkSemaphore, gMaxFramesInFlight> renderCompleteSemaphores;

			std::array<VkFence, gMaxFramesInFlight> inFlightFences;

			vk::SwapChain swapChain;

			vk::PipelineManager pipelineManager;

			VkRenderPass renderPass = VK_NULL_HANDLE;

			float FOV = 45.f;
			Camera mCamera;
			vk::UserInterface UIOverlay;

		

		public: 

			ContextBase(); /* expect this to be derived from */
			virtual ~ContextBase();

			//pure virtual function(s)
			virtual void RecordCommandBuffers() = 0;
			virtual void UpdateUI() = 0;
			virtual void ResizeWindowDerived() = 0;
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
			void ResizeWindow();

		protected:

			bool PrepareFrame();

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

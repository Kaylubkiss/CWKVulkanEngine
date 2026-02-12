#pragma once

#include "vkWindow.h"
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
		[[nodiscard]] VkPhysicalDevice PhysicalDevice() const;
		[[nodiscard]] VkDevice LogicalDevice() const;
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

	protected:
		bool PrepareFrame();
		//more pure virtual function(s)
		virtual void InitializePipeline( std::string vsFile, std::string fsFile ) = 0;
		virtual void InitializeDescriptors() = 0;
		//non-pure virtual functions
		virtual void InitializeRenderPass();
		virtual void FillOutGraphicsContextInfo();
	private:
		void CreateWindow();
		void CreateInstance();
		void CreateSynchronizationPrimitives();
	protected:
		std::shared_ptr<GraphicsContextInfo> m_info;//this is for textureManager and potentially any other discrete systems.
		//WARNING: context specific!!!

		struct Settings {
			uint32_t maxFramesInFlight = 2;
			bool minimized = false;
			bool UIEnabled = true;
			bool validationLayers = true;
			VkDebugUtilsMessengerEXT debugMessenger = nullptr;
		} settings;

		VkInstance instance = VK_NULL_HANDLE;
		VkRenderPass renderPass = VK_NULL_HANDLE;

		vk::Window window;
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

		std::unique_ptr<ObjectManager> m_objectManager; //application technically owns this pointer.
	};
}	

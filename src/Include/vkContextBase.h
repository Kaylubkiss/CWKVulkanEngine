#pragma once

#include "vkSwapChain.h"
#include "VkPipeline.h"
#include "HotReloader.h"
#include "vkDevice.h"

namespace vk
{

	/* NOTE: JANK FORWARD DECLARATION, BECAUSE OF A DOUBLE INCLUDE PROBABLY */
	class ObjectManager;

	class ContextBase
	{
		protected:
			vk::Window window;

			VkInstance instance = VK_NULL_HANDLE;

			Device device;

			vk::HotReloader mHotReloader;

			bool isInitialized = false;

			VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
			
			VkCommandPool commandPool = VK_NULL_HANDLE;
			std::vector<VkCommandBuffer> commandBuffers;

			VkExtent2D currentExtent = { 0,0 };

			struct RenderingSemaphores
			{
				VkSemaphore presentComplete;
				VkSemaphore renderComplete;

			} semaphores{};

			VkSubmitInfo submitInfo = {};

			VkPipelineStageFlags pipelineWaitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

			vk::SwapChain swapChain;

			vk::rsc::DepthStencil depthStencil;

			vk::Pipeline mPipeline;

		public: 

			ContextBase(); /* expect this to be derived from */
			virtual ~ContextBase();

			//pure virtual function(s)
			virtual void RecordCommandBuffers(vk::ObjectManager& objManager) = 0;
			virtual void ResizeWindow();
			virtual std::vector<VkWriteDescriptorSet> WriteDescriptorBuffers(VkDescriptorSet descriptorSet) = 0;
			virtual uint32_t SamplerDescriptorSetBinding() = 0;
			virtual const VkDescriptorSetLayout DescriptorSetLayout() const = 0;
			virtual void InitializeScene(ObjectManager& objManager) = 0;

			//virtual function(s)
			virtual void Render();

			//getter(s)
			vk::Queue GraphicsQueue();
			const VkPipeline Pipeline() const;
			const VkPhysicalDevice PhysicalDevice() const;
			const VkDevice LogicalDevice() const;
			VkDescriptorPool DescriptorPool() const;

			//operations
			void WaitForDevice();

		protected:
			//more pure virtual function(s)
			virtual void InitializePipeline(std::string vsFile = "", std::string fsFile = "") = 0;
			virtual void InitializeDescriptors() = 0;

			//non-pure virtual functions
			virtual void InitializeRenderPass();

			//non virtual function
			virtual void InitializeDepthStencil();

		private:
			VkDevice CreateLogicalDevice(const VkPhysicalDevice& p_device, uint32_t graphicsFamily, uint32_t presentFamily);

			void FindQueueFamilies(const VkSurfaceKHR& windowSurface);

			void EnumeratePhysicalDevices();

			void CreateWindow();
			void CreateInstance();
	};
}	

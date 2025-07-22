#pragma once

#include "vkSwapChain.h"
#include "vkRenderResources.h"
#include "VkPipeline.h"
#include "HotReloader.h"


namespace vk
{

	/* NOTE: JANK FORWARD DECLARATION, BECAUSE OF A DOUBLE INCLUDE PROBABLY */
	class ObjectManager;

	class ContextBase
	{
		protected:
			vk::Window window;

			VkInstance instance = VK_NULL_HANDLE;

			vk::UniformTransform uTransform;

			vk::HotReloader mHotReloader;

			bool isInitialized = false;

			std::shared_ptr<VkDescriptorPool> descriptorPool;
			
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

			struct Device
			{
				VkPhysicalDevice physical = VK_NULL_HANDLE;
				VkDevice logical = VK_NULL_HANDLE;
			} device{};

			vk::Queue graphicsQueue;
			vk::Queue presentQueue;

			vk::Pipeline mPipeline;

		public: 

			ContextBase(); /* expect this to be derived from */
			ContextBase(const ContextBase&) = delete;

			~ContextBase();

			//pure virtual function(s)
			virtual void RecordCommandBuffers(vk::ObjectManager& objManager) = 0;
			virtual void ResizeWindow() = 0;
			virtual std::vector<VkDescriptorBufferInfo> DescriptorBuffers() = 0;
			virtual void InitializeScene(ObjectManager& objManager) = 0;

			//getter(s)
			vk::Queue GraphicsQueue();
			const VkDescriptorSetLayout DescriptorSetLayout() const;
			const VkPipeline Pipeline() const;
			const VkPhysicalDevice PhysicalDevice() const;
			const VkDevice LogicalDevice() const;
			vk::UniformTransform& SceneTransform();
			std::shared_ptr<VkDescriptorPool> DescriptorPool() const;

			void WaitForDevice();

			void Render();

			void BindPipelineLayoutToObject(Object& obj);

		protected:
			virtual void InitializePipeline(std::string vsFile, std::string fsFile) = 0;
			virtual void InitializeDescriptorPool() = 0;

		private:
			VkDevice CreateLogicalDevice(const VkPhysicalDevice& p_device, uint32_t graphicsFamily, uint32_t presentFamily);

			void FindQueueFamilies(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& windowSurface);

			void EnumeratePhysicalDevices();

			void CreateWindow();
			void CreateInstance();
	};
}	

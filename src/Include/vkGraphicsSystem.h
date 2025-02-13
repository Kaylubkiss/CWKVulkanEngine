#pragma once

#include "Common.h"
#include "vkSwapChain.h"

namespace vk
{

	struct RenderResources
	{
		VkFence inFlightFence = VK_NULL_HANDLE;

		VkCommandPool commandPool = VK_NULL_HANDLE;
		VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

		VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
		VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;


		VkPipeline defaultPipeline = VK_NULL_HANDLE;

		void Destroy(const VkDevice l_device);
	};

	class GraphicsSystem
	{
		private:
			//may need to create array system out of this.
			//for now, just keep it to one logical and physical device.
			RenderResources renderResources;

			vk::SwapChain swapChain;
			
			VkPhysicalDevice* gpus;
			unsigned int g_index = -1;

			VkDevice logicalGpu;

			Queue graphicsQueue;
			Queue presentQueue;


		public:
			GraphicsSystem(const VkInstance vkInstance, const VkSurfaceKHR windowSurface);
			~GraphicsSystem();

			const VkPhysicalDevice PhysicalDevice() const;
			const VkDevice LogicalDevice() const;

			static VkDevice CreateLogicalDevice(const VkPhysicalDevice& p_device, uint32_t graphicsFamily, uint32_t presentFamily);


		private:
			GraphicsSystem() = default;

			void FindQueueFamilies(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& windowSurface);

			void EnumeratePhysicalDevices(const VkInstance& vkInstance);
	};
}

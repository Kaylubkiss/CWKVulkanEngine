#pragma once

#include "Common.h"
#include "vkSwapChain.h"

namespace vk
{
	struct RenderResources
	{
		uTransformObject uTransform = {};
		
		VkFence inFlightFence = VK_NULL_HANDLE;

		VkCommandPool commandPool = VK_NULL_HANDLE;
		VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

		VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
		VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;

		
		vk::Buffer uniformBuffer;

		VkRenderPass renderPass;

		vk::DepthResources depthInfo;

		//for window size information;
		VkExtent2D currentExtent;
	
		//pipeline information
		VkDescriptorSetLayout defaultDescriptorSetLayout = VK_NULL_HANDLE;
		VkPipelineLayout defaultPipelineLayout = VK_NULL_HANDLE;
		VkPipeline defaultPipeline = VK_NULL_HANDLE;

		VkShaderModule fragmentShaderModule = VK_NULL_HANDLE;
		VkShaderModule vertexShaderModule = VK_NULL_HANDLE;

		void Allocate(const VkPhysicalDevice p_device, const VkDevice l_device, const vk::Window& appWindow);

		void Destroy(const VkDevice l_device);
	};

	class GraphicsSystem
	{
		private:
			//may need to create array system out of this.
			//for now, just keep it to one logical and physical device.
			vk::RenderResources renderResources;

			vk::SwapChain swapChain;
			
			VkPhysicalDevice* gpus;
			unsigned int g_index = -1;

			VkDevice logicalGpu;

			vk::Queue graphicsQueue;
			vk::Queue presentQueue;


		public:
			GraphicsSystem(const VkInstance vkInstance, const vk::Window& appWindow);
			GraphicsSystem() = default;
			GraphicsSystem(const GraphicsSystem&) = delete;

			void Destroy();
			~GraphicsSystem() = delete;


			const VkPhysicalDevice PhysicalDevice() const;
			const VkDevice LogicalDevice() const;

			static VkDevice CreateLogicalDevice(const VkPhysicalDevice& p_device, uint32_t graphicsFamily, uint32_t presentFamily);

			void UpdateUniformViewMatirx(const glm::mat4& viewMat);
			void ResizeWindow();

			void Render();
		private:
			GraphicsSystem() = default;

			void FindQueueFamilies(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& windowSurface);

			void EnumeratePhysicalDevices(const VkInstance& vkInstance);
	};
}

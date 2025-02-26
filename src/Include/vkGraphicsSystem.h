#pragma once

#include "vkSwapChain.h"
#include "vkRenderResources.h"

namespace vk
{
	class GraphicsSystem
	{
		private:
			//may need to create array system out of this.
			//for now, just keep it to one logical and physical device.
			vk::RenderResources renderResources;

			vk::SwapChain swapChain;
			
			VkPhysicalDevice* gpus = nullptr;
			unsigned int g_index = -1;

			VkDevice logicalGpu = VK_NULL_HANDLE;

			vk::Queue graphicsQueue;
			vk::Queue presentQueue;


		public:
			GraphicsSystem(const VkInstance vkInstance, const vk::Window& appWindow);
			GraphicsSystem() = default;
			GraphicsSystem(const GraphicsSystem&) = delete;

			void Destroy();
			~GraphicsSystem() = default;


			const VkPhysicalDevice PhysicalDevice() const;
			const VkDevice LogicalDevice() const;
			const VkQueue GraphicsQueue() const;
			const VkDescriptorSetLayout DescriptorSetLayout() const;
			const VkRenderPass RenderPass() const;
			const VkPipeline Pipeline() const;
			const VkBuffer UniformTransformBuffer() const;

			static VkDevice CreateLogicalDevice(const VkPhysicalDevice& p_device, uint32_t graphicsFamily, uint32_t presentFamily);

			void UpdateUniformViewMatrix(const glm::mat4& viewMat);

			void ResizeWindow();

			void WaitForQueueSubmission();

			void Render(const vk::Window& appWindow, VkCommandBuffer* secondCmdBuffers, size_t secondCmdCount);

			VkCommandPool CommandPool();

			void BindPipelineLayoutToObject(Object& obj);

		private:
			void FindQueueFamilies(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& windowSurface);

			void EnumeratePhysicalDevices(const VkInstance& vkInstance);
	};
}

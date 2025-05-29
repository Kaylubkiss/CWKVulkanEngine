#pragma once

#include "vkSwapChain.h"
#include "vkRenderResources.h"
#include "VkPipeline.h"
#include "HotReloader.h"



namespace vk
{
	class GraphicsSystem
	{
		private:
			bool isInitialized = false;

			uTransformObject uTransform;
			vk::Buffer uTransformBuffer;

			vk::uLightObject uLight;
			vk::Buffer uLightBuffer;

			//may need to create array system out of this.
			//for now, just keep it to one logical and physical device.
			vk::RenderResources renderResources;

			//maybe one day make a map of pipelines??
			vk::Pipeline mPipeline;

			vk::SwapChain swapChain;

			std::vector<VkPhysicalDevice> gpus;
			int g_index = -1;

			VkDevice logicalGpu = VK_NULL_HANDLE;

			vk::Queue graphicsQueue;
			vk::Queue presentQueue;


		public:
			GraphicsSystem(const VkInstance vkInstance, const vk::Window& appWindow);

			GraphicsSystem() = default;
			GraphicsSystem(const GraphicsSystem&) = delete;

			void Destroy();
			~GraphicsSystem() = default;

			//getters
			const VkPhysicalDevice PhysicalDevice() const;
			const VkDevice LogicalDevice() const;
			vk::Queue GraphicsQueue();
			const VkDescriptorSetLayout DescriptorSetLayout() const;
			VkRenderPass RenderPass();
			const VkPipeline Pipeline() const;
			VkCommandPool CommandPool();
			const vk::Buffer& UTransformBuffer();
			const vk::Buffer& ULightBuffer();
			void UpdateUniformViewMatrix(const glm::mat4& viewMat);

			void ResizeWindow();

			void WaitForQueueSubmission();
			void WaitForDevice();

			void Render(const vk::Window& appWindow, VkCommandBuffer* secondCmdBuffers, size_t secondCmdCount);

			void BindPipelineLayoutToObject(Object& obj);

			void AttachHotReloader(HotReloader& hotReloader) 
			{
				hotReloader = HotReloader(this->logicalGpu, this->mPipeline);
			}

		private:

			VkDevice CreateLogicalDevice(const VkPhysicalDevice& p_device, uint32_t graphicsFamily, uint32_t presentFamily);

			void FindQueueFamilies(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& windowSurface);

			void EnumeratePhysicalDevices(const VkInstance& vkInstance);
	};
}

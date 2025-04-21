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

			uTransformObject uTransform;
			vk::Buffer uTransformBuffer;

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
			inline GraphicsSystem& operator=(const GraphicsSystem& other) 
			{
				if (this == &other) {
					return *this;
				}

				uTransform = other.uTransform;
				uTransformBuffer = other.uTransformBuffer;
				renderResources = other.renderResources;
				mPipeline = other.mPipeline;
				swapChain = other.swapChain;
				gpus = other.gpus;
				g_index = other.g_index;
				logicalGpu = other.logicalGpu;
				graphicsQueue = other.graphicsQueue;
				presentQueue = other.presentQueue;

				return *this;
			}

			GraphicsSystem() = default;
			GraphicsSystem(const GraphicsSystem&) = delete;

			void Destroy();
			~GraphicsSystem() = default;

			//getters
			const VkPhysicalDevice PhysicalDevice() const;
			const VkDevice LogicalDevice() const;
			vk::Queue GraphicsQueue();
			const VkDescriptorSetLayout DescriptorSetLayout() const;
			const VkRenderPass RenderPass() const;
			const VkPipeline Pipeline() const;
			VkCommandPool CommandPool();
			const VkBuffer UniformTransformBuffer();
			void UpdateUniformViewMatrix(const glm::mat4& viewMat);

			void ResizeWindow();

			void WaitForQueueSubmission();

			void Render(const vk::Window& appWindow, VkCommandBuffer* secondCmdBuffers, size_t secondCmdCount);

			void BindPipelineLayoutToObject(Object& obj);

			void AttachHotReloader(HotReloader& hotReloader) 
			{
				hotReloader = HotReloader(&this->logicalGpu, this->mPipeline, &this->renderResources.renderPass);
			}

		private:

			VkDevice CreateLogicalDevice(const VkPhysicalDevice& p_device, uint32_t graphicsFamily, uint32_t presentFamily);

			void FindQueueFamilies(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& windowSurface);

			void EnumeratePhysicalDevices(const VkInstance& vkInstance);
	};
}

#pragma once

#include "vkSwapChain.h"
#include "vkRenderResources.h"
#include "VkPipeline.h"
#include "HotReloader.h"


namespace vk
{

	//JANK FORWARD DECLARATION, BECAUSE OF A DOUBLE INCLUDE PROBABLY
	class ObjectManager;


	class GraphicsSystem
	{
	private:
		bool isInitialized = false;

		uTransformObject uTransform;
		vk::Buffer uTransformBuffer;

		uLightObject uLight;
		vk::Buffer uLightBuffer;

		//may need to create array system out of this.
		//for now, just keep it to one logical and physical device.
		vk::RenderResources renderResources;
	
		//maybe one day make a map of pipelines??
		vk::Pipeline mPipeline;

		struct RenderingSemaphores
		{
			VkSemaphore presentComplete;
			VkSemaphore renderComplete;

		} semaphores{};

		VkPipelineStageFlags pipelineWaitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkSubmitInfo submitInfo = {};

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

			~GraphicsSystem() = default;
			void Destroy();

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
			void UpdateUniformModelMatrix(const glm::mat4& modelMat);

			void ResizeWindow();

			void WaitForDevice();

			void Render(const vk::Window& appWindow, VkCommandBuffer* secondCmdBuffers, size_t secondCmdCount);

			void BindPipelineLayoutToObject(Object& obj);

			void AttachHotReloader(HotReloader& hotReloader) 
			{
				hotReloader = HotReloader(this->logicalGpu, this->mPipeline);
			}

			void BuildCommandBuffers(vk::ObjectManager& objManager);

		private:

			void InitializePipeline(const vk::Window& appWindow);
			void InitializedUniforms(const vk::Window& appWindow);

			VkDevice CreateLogicalDevice(const VkPhysicalDevice& p_device, uint32_t graphicsFamily, uint32_t presentFamily);

			void FindQueueFamilies(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& windowSurface);

			void EnumeratePhysicalDevices(const VkInstance& vkInstance);
	};
}	

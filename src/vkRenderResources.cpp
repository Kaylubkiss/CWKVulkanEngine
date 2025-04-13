#include "vkRenderResources.h"
#include "ApplicationGlobal.h"
#include "vkInit.h"
#ifdef _DEBUG
#include "SpirvHelper.h"
#endif
#include "vkUtility.h"


namespace vk 
{
	



	void RenderResources::Allocate(const VkPhysicalDevice p_device, const VkDevice l_device, const vk::Window& appWindow)
	{
		assert(_Application != NULL);

		Camera& appCamera = _Application->GetCamera();

		this->commandPool = vk::init::CommandPool(l_device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		this->commandBuffer = vk::init::CommandBuffer(l_device, this->commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

		//uniform transform for objects of default pipeline.
		global::uTransform = {
			appCamera.LookAt(), //view
			glm::perspective(glm::radians(45.f), (float)appWindow.viewport.width / appWindow.viewport.height, 0.1f, 1000.f) //proj
		};

		global::uTransform.proj[1][1] *= -1.f;

		//uniform(s)
		global::uTransformBuffer = vk::Buffer(p_device, l_device, sizeof(uTransformObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, (void*)&global::uTransform);

		//semaphores
		this->imageAvailableSemaphore = vk::init::CreateSemaphore(l_device);
		this->renderFinishedSemaphore = vk::init::CreateSemaphore(l_device);

		//fence(s)
		this->inFlightFence = vk::init::CreateFence(l_device);

		this->depthInfo = vk::rsc::CreateDepthResources(p_device, l_device, appWindow.viewport);

		this->renderPass = vk::init::RenderPass(l_device, this->depthInfo.depthFormat);

		//window sizing...
		VkSurfaceCapabilitiesKHR deviceCapabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(p_device, appWindow.surface, &deviceCapabilities);

		this->currentExtent = deviceCapabilities.currentExtent;
	}

	void RenderResources::Destroy(const VkDevice l_device)
	{
		//pipeline info...

		vkDestroyRenderPass(l_device, this->renderPass, nullptr);

		this->depthInfo.Destroy(l_device);

		vkDestroyFence(l_device, this->inFlightFence, nullptr);

		//command pools and their handles.
		vkFreeCommandBuffers(l_device, this->commandPool, 1, &this->commandBuffer);
		vkDestroyCommandPool(l_device, this->commandPool, nullptr);

		//semaphores
		vkDestroySemaphore(l_device, this->imageAvailableSemaphore, nullptr);
		vkDestroySemaphore(l_device, this->renderFinishedSemaphore, nullptr);

		//buffers
		vkDestroyBuffer(l_device, global::uTransformBuffer.handle, nullptr);
		vkFreeMemory(l_device, global::uTransformBuffer.memory, nullptr);
	}
}

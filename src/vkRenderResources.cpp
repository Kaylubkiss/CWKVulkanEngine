#include "vkRenderResources.h"
#include "ApplicationGlobal.h"
#include "vkInit.h"
#include "SpirvHelper.h"
#include "vkUtility.h"


namespace vk 
{
	



	void RenderResources::Allocate(const VkPhysicalDevice p_device, const VkDevice l_device, const vk::Window& appWindow)
	{

		this->commandPool = vk::init::CommandPool(l_device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		this->commandBuffer = vk::init::CommandBuffer(l_device, this->commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

		//semaphores
		this->imageAvailableSemaphore = vk::init::CreateSemaphore(l_device);
		this->renderFinishedSemaphore = vk::init::CreateSemaphore(l_device);

		//fence(s)
		this->inFlightFence = vk::init::CreateFence(l_device);

		/*this->depthInfo = vk::rsc::CreateDepthResources(p_device, l_device, appWindow.viewport);

		this->renderPass = vk::init::RenderPass(l_device, this->depthInfo.depthFormat);*/

		//window sizing...
		VkSurfaceCapabilitiesKHR deviceCapabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(p_device, appWindow.surface, &deviceCapabilities);

		this->currentExtent = deviceCapabilities.currentExtent;
	}

	void RenderResources::Destroy(const VkDevice l_device)
	{
		//pipeline info...

		vkDestroyFence(l_device, this->inFlightFence, nullptr);

		//command pools and their handles.
		vkFreeCommandBuffers(l_device, this->commandPool, 1, &this->commandBuffer);
		vkDestroyCommandPool(l_device, this->commandPool, nullptr);

		//semaphores
		vkDestroySemaphore(l_device, this->imageAvailableSemaphore, nullptr);
		vkDestroySemaphore(l_device, this->renderFinishedSemaphore, nullptr);

		
	}
}

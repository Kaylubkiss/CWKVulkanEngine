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

		VkSurfaceCapabilitiesKHR deviceCapabilities;
		VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(p_device, appWindow.surface, &deviceCapabilities));

		//size of command buffer array is the same as swap chain image array
		for (int i = 0; i < deviceCapabilities.minImageCount + 1; ++i) 
		{
			this->commandBuffers.push_back(vk::init::CommandBuffer(l_device, this->commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));
		}

		//semaphores
		/*this->imageAvailableSemaphore = vk::init::CreateSemaphore(l_device);
		this->renderFinishedSemaphore = vk::init::CreateSemaphore(l_device);*/

		//fence(s)
		this->inFlightFence = vk::init::CreateFence(l_device);

		/*this->depthInfo = vk::rsc::CreateDepthResources(p_device, l_device, appWindow.viewport);

		this->renderPass = vk::init::RenderPass(l_device, this->depthInfo.depthFormat);*/

		this->currentExtent = deviceCapabilities.currentExtent;
	}

	void RenderResources::Destroy(const VkDevice l_device)
	{
		//pipeline info...

		vkDestroyFence(l_device, this->inFlightFence, nullptr);

		//command pools and the buffers allocated.
		vkFreeCommandBuffers(l_device, this->commandPool, this->commandBuffers.size(), this->commandBuffers.data());

		vkDestroyCommandPool(l_device, this->commandPool, nullptr);


		
	}
}

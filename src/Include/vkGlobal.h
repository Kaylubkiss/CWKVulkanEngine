#pragma once

#include "Common.h"


namespace vk
{

	const char* enabledLayerNames[1] =
	{
		"VK_LAYER_KHRONOS_validation"
	};

	const char* deviceExtensions[1] =
	{
		"VK_KHR_swapchain"
	};

	struct uTransformObject
	{
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};


	struct DepthResources
	{
		VkImage depthImage;
		VkDeviceMemory depthImageMemory;
		VkImageView depthImageView;
		VkFormat depthFormat;

		const DepthResources& operator=(const DepthResources& other) 
		{
			if (this != &other) 
			{
				this->depthImage = other.depthImage;
				this->depthImageMemory = other.depthImageMemory;
				this->depthImageView = other.depthImageView;
				this->depthFormat = other.depthFormat;
			}

			return *this;
		}

		void Destroy(const VkDevice l_device) 
		{
			vkDestroyImage(l_device, this->depthImage, nullptr);
			vkDestroyImageView(l_device, this->depthImageView, nullptr);
			vkFreeMemory(l_device, this->depthImageMemory, nullptr);
		}
	};

	struct Queue 
	{
		VkQueue handle;
		uint32_t family;
	};

	VkCommandBuffer beginCmd(const VkDevice l_device, const VkCommandPool cmdPool);

	void endCmd(const VkDevice l_device, VkCommandBuffer commandBuffer, const VkCommandPool cmdPool, const VkQueue gfxQueue);


}
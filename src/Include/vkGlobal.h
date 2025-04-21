#pragma once

#include <vulkan/vulkan.h>
#include <vkBuffer.h>

namespace vk
{
	

	static const char* instanceExtensions[1] =
	{
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME
	};

	static const char* instanceLayerExtensions[1] = 
	{
		"VK_LAYER_KHRONOS_validation"
	};

	static const char* deviceExtensions[1] =
	{
		"VK_KHR_swapchain"
	};

	struct uTransformObject
	{
		glm::mat4 view;
		glm::mat4 proj;
	};


	struct Queue 
	{
		VkQueue handle = VK_NULL_HANDLE;
		uint32_t family = -1;
	};

	VkCommandBuffer beginSingleTimeCommand(const VkDevice l_device, const VkCommandPool cmdPool);

	void endSingleTimeCommand(const VkDevice l_device, VkCommandBuffer commandBuffer, const VkCommandPool cmdPool, const VkQueue gfxQueue);

	/*namespace global
	{
		extern uTransformObject uTransform;
		extern vk::Buffer uTransformBuffer;
	}

	void UpdateUniformViewMatrix(const glm::mat4& viewMat);*/
}
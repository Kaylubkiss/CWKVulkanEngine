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
		glm::mat4 view = glm::mat4(1.f);
		glm::mat4 proj = glm::mat4(1.f);
		glm::vec3 camPosition = glm::vec3(0.f);
	};


	struct uLightObject 
	{
		glm::vec3 pos;
		glm::vec3 albedo;
		glm::vec3 ambient;
		glm::vec3 specular;
		float shininess;
	};

	struct Queue 
	{
		VkQueue handle = VK_NULL_HANDLE;
		uint32_t family = -1;
	};

	VkCommandBuffer beginSingleTimeCommand(const VkDevice l_device, const VkCommandPool cmdPool);

	void endSingleTimeCommand(const VkDevice l_device, VkCommandBuffer commandBuffer, const VkCommandPool cmdPool, const VkQueue gfxQueue);
}
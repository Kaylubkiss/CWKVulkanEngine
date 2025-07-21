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

	

	struct Queue 
	{
		VkQueue handle = VK_NULL_HANDLE;
		uint32_t family = -1;
	};

	VkCommandBuffer beginSingleTimeCommand(const VkDevice l_device, const VkCommandPool cmdPool);

	void endSingleTimeCommand(const VkDevice l_device, VkCommandBuffer commandBuffer, const VkCommandPool cmdPool, const VkQueue gfxQueue);
}

//these don't need to be tied to the vulkan API!!!
struct uTransformObject
{
	glm::mat4 view = glm::mat4(1.f);
	glm::mat4 proj = glm::mat4(1.f);
};


struct uLightObject
{
	glm::vec3 pos; /* position of light */
	glm::vec3 ambient; /* scene color */
	glm::vec3 albedo; /* base color of light */
	glm::vec3 specular; /* reflectivity of the light */
	float shininess; /* exponent value */
};
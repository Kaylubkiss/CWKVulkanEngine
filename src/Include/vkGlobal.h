#pragma once

#include <vulkan/vulkan.h>
#include <vkBuffer.h>


//these don't need to be tied to the vulkan API!!!
struct uTransformObject
{
	glm::mat4 view = glm::mat4(1.f);
	glm::mat4 proj = glm::mat4(1.f);
};


struct uLightObject
{
	glm::vec3 pos = glm::vec3(0.f); /* position of light */
	glm::vec3 ambient = glm::vec3(0.f); /* scene color */
	glm::vec3 albedo = glm::vec3(0.f); /* base color of light */
	glm::vec3 specular = glm::vec3(0.f); /* reflectivity of the light */
	float shininess = 0.f; /* exponent value */
};

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

	struct UniformTransform 
	{
		uTransformObject data;
		vk::Buffer buffer;
	};

	VkCommandBuffer beginSingleTimeCommand(const VkDevice l_device, const VkCommandPool cmdPool);

	void endSingleTimeCommand(const VkDevice l_device, VkCommandBuffer commandBuffer, const VkCommandPool cmdPool, const VkQueue gfxQueue);
}


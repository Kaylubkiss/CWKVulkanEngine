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

	struct FramebufferAttachment
	{
		VkImage image = VK_NULL_HANDLE;
		VkDeviceMemory imageMemory = VK_NULL_HANDLE;
		VkImageView imageView = VK_NULL_HANDLE;
		VkFormat format = {};

		void Destroy(VkDevice l_device)
		{
			vkDestroyImageView(l_device, this->imageView, nullptr);
			vkDestroyImage(l_device, this->image, nullptr);
			vkFreeMemory(l_device, this->imageMemory, nullptr);
		}
	};

	VkCommandBuffer beginSingleTimeCommand(const VkDevice l_device, const VkCommandPool cmdPool);

	void endSingleTimeCommand(const VkDevice l_device, VkCommandBuffer commandBuffer, const VkCommandPool cmdPool, const VkQueue gfxQueue);
}


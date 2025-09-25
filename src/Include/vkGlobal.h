#pragma once

#include <vulkan/vulkan.h>
#include <vkBuffer.h>
#include <queue>

//these don't need to be tied to the vulkan API!!!
struct uTransformObject
{
	glm::mat4 view = glm::mat4(1.f);
	glm::mat4 proj = glm::mat4(1.f);
};


struct uLightObject
{
	float shininess = 0.f; /* exponent value */

	glm::vec3 pos = glm::vec3(0.f); /* position of light */
	glm::vec3 ambient = glm::vec3(0.f); /* scene color */
	glm::vec3 albedo = glm::vec3(0.f); /* base color of light */
	glm::vec3 specular = glm::vec3(0.f); /* reflectivity of the light */
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
		uint32_t family = 0;
	};

	struct UniformTransform 
	{
		uTransformObject data;
		vk::Buffer buffer;
	};

	//created in response to the need of texture manager. It needs a lot of graphics context state, but the calls to 
	//function methods of the context to get this information seemed inconvenient.
	//in turn, I've had to create this data structure which contains all the information that
	//texture manager needs of the current context.
	//it's a little janky.
	struct GraphicsContextInfo 
	{
		std::vector<VkWriteDescriptorSet> sceneWriteDescriptorSets;
		VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
		VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
		VkDevice logicalDevice = VK_NULL_HANDLE;
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		vk::Queue graphicsQueue = {};
		uint32_t samplerBinding = 0; //TODO: what if there are multiple sampler bindings across different objects?
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


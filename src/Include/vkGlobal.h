#pragma once

#include "Vertex.h"
#include "vkBuffer.h"
#include "vkDevice.h"
#include "vkTexture.h"
#include "UserInterface.h"

const uint32_t gMaxFramesInFlight = 3;

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

	//created in response to the need of texture manager. It needs a lot of graphics context state, but the calls to 
	//function methods of the context to get this information seemed inconvenient.
	//in turn, I've had to create this data structure which contains all the information that
	//texture manager needs of the current context.
	//it's a little janky.
	struct GraphicsContextInfo
	{
		uint32_t object_count = 0;
		vk::Device* devicePtr = nullptr;
		vk::UserInterface* contextUIPtr = nullptr;
		VkDeviceSize textureBindingSize = 0;
	};


	VkCommandBuffer beginSingleTimeCommand(const VkDevice l_device, const VkCommandPool cmdPool);

	void endSingleTimeCommand(const VkDevice l_device, VkCommandBuffer commandBuffer, const VkCommandPool cmdPool, const VkQueue gfxQueue);
}





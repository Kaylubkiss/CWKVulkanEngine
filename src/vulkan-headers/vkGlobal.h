#pragma once

#include "Vertex.h"
#include "vkBuffer.h"
#include "vkDevice.h"
#include "vkTexture.h"

constexpr uint32_t gMaxFramesInFlight = 3;

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

struct Primitive
{
	uint32_t firstIndex = 0;
	uint32_t indexCount = 0;

	uint32_t firstVertex = 0;
	uint32_t vertexCount = 0;

	std::optional<uint32_t> textureIndex = std::nullopt;
};

struct Mesh
{
	std::string m_name;
	std::vector<Primitive> m_primitives;
	Mesh() = default;
	Mesh(const std::string& name, const std::vector<Primitive>& primitives)
	{
		m_name = name;
		m_primitives = primitives;
	}
};

class UserInterface;

namespace vk
{
	class Buffer;
	class Device;

	struct DescriptorBufferData //240 BYTES!!!
	{
		std::array<vk::Buffer, gMaxFramesInFlight> buffers; //descriptors are stored in BUFFERS, not VkDescriptorSet
		std::vector<VkDeviceSize> binding_offsets = { 0ull }; //at least 1 binding (binding 0)
		
		VkDescriptorSetLayout layout = VK_NULL_HANDLE;
		VkDevice c_device            = VK_NULL_HANDLE;

		VkDeviceSize size = 0ull;

		void Destroy()
		{
			for (auto& b : buffers)
			{
				b.Destroy();
			}
			vkDestroyDescriptorSetLayout(c_device, layout, nullptr);
		}
	};

	//created in response to the need of texture manager. It needs a lot of graphics context state, but the calls to 
	//function methods of the context to get this information seemed inconvenient.
	//in turn, I've had to create this data structure which contains all the information that
	//texture manager needs of the current context.
	//it's a little janky.
	struct GraphicsContextInfo
	{
		vk::Device* devicePtr = nullptr;
		UserInterface* contextUIPtr = nullptr;
		DescriptorBufferData* contextTextureDescriptorPtr = nullptr;
	};

	//This allows the user to pass in relevant arguments to draw an object.
	struct DrawInfo
	{
		VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		VkDeviceSize bufferOffset = 0; //just in case I ever decide to make a more compact buffer.
		VkDeviceSize textureBindingSize = 0; //used for buffer offset calculation
		uint32_t imageBufferIndex = 0;
		uint32_t firstSet = 0;
		uint32_t setCount = 1; //it would make sense that there is at least 1 set being described.
		bool sampleTexture = false;
	};


	VkCommandBuffer beginSingleTimeCommand(const VkDevice l_device, const VkCommandPool cmdPool);

	void endSingleTimeCommand(const VkDevice l_device, VkCommandBuffer commandBuffer,
		const VkCommandPool cmdPool, const VkQueue gfxQueue);
}





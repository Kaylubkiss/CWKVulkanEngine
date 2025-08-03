#pragma once


#include "vkBuffer.h"
#include "Vertex.h"

namespace vk 
{
	struct MeshBuffers 
	{
		vk::Buffer vertex;
		vk::Buffer index;
	};
}

struct MeshData
{
	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;
};

struct Mesh
{

	vk::MeshBuffers buffer;
	MeshData data;

	glm::mat4  modelTransform = glm::mat4(1.f);

	glm::vec3 center = glm::vec3(0.f);

	glm::vec3 maxLocalPoints = glm::vec3(0.f);
	glm::vec3 minLocalPoints = glm::vec3(0.f);

	void Destroy(const VkDevice l_device) 
	{
		this->buffer.vertex.Destroy();
		this->buffer.index.Destroy();
	}
};

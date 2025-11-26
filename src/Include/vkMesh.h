#pragma once


namespace vk 
{
	struct MeshBuffers 
	{
		vk::Buffer vertex;
		uint32_t indexCount = 0;
		vk::Buffer index;
	};
}

struct Mesh
{

	vk::MeshBuffers buffer;

	glm::vec3 center = glm::vec3(0.f);

	glm::vec3 maxLocalPoints = glm::vec3(0.f);
	glm::vec3 minLocalPoints = glm::vec3(0.f);

	bool LoadOBJMesh(const char* filePath);
	void ComputeVertices(std::vector<Vertex>& vertexBuffer, std::vector<uint16_t>& indexBuffer);
	void ComputeVertexNormals(std::vector<Vertex>& vertexBuffer, std::vector<uint16_t>& indexBuffer);

	void Destroy(const VkDevice l_device) 
	{
		this->buffer.vertex.Destroy();
		this->buffer.index.Destroy();
	}
};

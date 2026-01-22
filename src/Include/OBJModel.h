#pragma once

#include "IModel.h"

class OBJModel : public IModel
{
public:
	OBJModel() = default;
	OBJModel( vk::Device* device, const std::filesystem::path& filePath );
	~OBJModel() = default;

	virtual glm::vec3 GetMinPoint() const override;
	virtual glm::vec3 GetMaxPoint() const override;

	virtual void UpdateModelTransform(const glm::mat4& newModelMatrix) override;
	virtual void Draw(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout = VK_NULL_HANDLE) override;
private:
	void ComputeVertices(std::vector<Vertex>& vertexBuffer, std::vector<uint16_t>& indexBuffer);
	void ComputeVertexNormals(std::vector<Vertex>& vertexBuffer, std::vector<uint16_t>& indexBuffer);
private:	
	glm::mat4 m_modelTransform  = glm::mat4(1.f);
	glm::vec3 m_center          = glm::vec3(0.f);
	glm::vec3 m_maxLocalPoint   = glm::vec3(0.f);
	glm::vec3 m_minLocalPoint   = glm::vec3(0.f);	
};

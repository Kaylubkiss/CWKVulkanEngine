#pragma once
#include "vkModel.h"

class OBJModel : public vk::Model
{
public:
	OBJModel() = default;
	OBJModel( vk::Device* device, const std::filesystem::path& filePath );
	~OBJModel() override = default;

	[[nodiscard]] glm::vec3 GetMinPoint() const override;
	[[nodiscard]] glm::vec3 GetMaxPoint() const override;

	void UpdateModelTransform(const glm::mat4& newModelMatrix) override;
	void Draw( const vk::DrawInfo& drawInfo ) override;
	void LoadTextures( TextureManager& textureManager, const std::vector<std::string>& textureNames ) override;
private:
	void ComputeVertices(std::vector<Vertex>& vertexBuffer, std::vector<uint16_t>& indexBuffer);
	void ComputeVertexNormals(std::vector<Vertex>& vertexBuffer, std::vector<uint16_t>& indexBuffer);
private:	
	glm::mat4 m_modelTransform  = glm::mat4(1.f);
	glm::vec3 m_center          = glm::vec3(0.f);
	glm::vec3 m_maxLocalPoint   = glm::vec3(0.f);
	glm::vec3 m_minLocalPoint   = glm::vec3(0.f);	
};

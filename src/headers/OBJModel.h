#ifndef OBJ_MODEL_HPP
#define OBJ_MODEL_HPP

#include "Model.h"

class OBJModel : public Model
{
public:
	OBJModel() = default;
	OBJModel( vk::Device* device, const std::filesystem::path& filePath );
	~OBJModel() override = default;

	[[nodiscard]] glm::vec3 GetMinPoint() const override;
	[[nodiscard]] glm::vec3 GetMaxPoint() const override;

	void Draw( const vk::DrawInfo& drawInfo ) override;
	void LoadTextures( vk::TextureManager& textureManager, const std::vector<std::string>& textureNames ) override;
private:
	void ComputeVertices( std::vector<Vertex>& vertexBuffer, std::vector<uint16_t>& indexBuffer );
	void ComputeVertexNormals( std::vector<Vertex>& vertexBuffer, std::vector<uint16_t>& indexBuffer );
private:
	glm::vec3 m_center          = glm::vec3(0.f);
	glm::vec3 m_maxLocalPoint   = glm::vec3(0.f);
	glm::vec3 m_minLocalPoint   = glm::vec3(0.f);	
};

#endif

#pragma once
#include "Model.h"
#include <fastgltf/types.hpp>

class GLTFModel : public Model
{
public:
	GLTFModel() = default;
	~GLTFModel() override = default;

	/*[[nodiscard]] glm::vec3 GetMinPoint() const override{ return glm::vec3(0); };
	[[nodiscard]] glm::vec3 GetMaxPoint() const override { return glm::vec3(0); };*/

	void UpdateModelTransform( const glm::mat4& newModelMatrix ) override; //no physics for GLTF yet.
	void Draw( const vk::DrawInfo& drawInfo ) override;
	//void LoadTextures( TextureManager& textureManager, const std::vector<std::string>& textureNames ) override;
protected:
	bool doLoad( vk::Device* devicePtr, ResourceManager& resourceManager ) override;
	void doUnload( vk::Device* devicePtr ) override {(void)(devicePtr);};
private:
	void LoadMeshes( fastgltf::Asset& asset, std::vector<Vertex>& vertexBuffer,
		std::vector<uint32_t>& indexBuffer );
	std::string LoadImage( vk::Device* devicePtr, fastgltf::Image& image );
private:
	glm::mat4 m_modelMatrix = glm::mat4(1.0f);
	VkIndexType m_indexBufferType = VK_INDEX_TYPE_UINT16;



};

#pragma once
//NOTE (11/5/25): JUST SUPPORTING MESH AND TEXTURES FOR NOW
#include "vkModel.h"


class GLTFModel : public vk::Model
{
public:
	GLTFModel() = default;
	GLTFModel( vk::Device* device, const std::filesystem::path& filePath );
	~GLTFModel() override = default;
	[[nodiscard]] glm::vec3 GetMinPoint() const override{ return glm::vec3(0); };
	[[nodiscard]] glm::vec3 GetMaxPoint() const override { return glm::vec3(0); };
	[[nodiscard]] std::vector<std::string> GetTextureFileNames() const; //unique to GLTFModel since it specifies many different textures.
	void UpdateModelTransform( const glm::mat4& newModelMatrix ) override { }; //no physics for GLTF yet.
	void Draw( const vk::DrawInfo& drawInfo ) override;
	void LoadTextures( TextureManager& textureManager, const std::vector<std::string>& textureNames ) override;
private:
	//helpers
	void LoadMesh( fastgltf::Mesh& mesh, std::vector<Vertex>& vertexBuffer, std::vector<uint16_t>& indexBuffer );
	std::string LoadImage( vk::Device* devicePtr, fastgltf::Image& image );
private:
	fastgltf::Asset m_asset; //TODO: move this out because it's too big.
	std::vector<std::shared_ptr<vk::Texture>> m_textures;



};

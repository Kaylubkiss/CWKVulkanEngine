#ifndef GLTF_MODEL_HPP
#define GLTF_MODEL_HPP

//NOTE (11/5/25): JUST SUPPORTING MESH AND TEXTURES FOR NOW
#include "Model.h"

#include <fastgltf/types.hpp>

//this is to ensure that we can memcpy, as the UpdateModelTransform() takes in a glm::mat4.
static_assert(sizeof(fastgltf::math::fmat4x4) == sizeof(glm::mat4));

class GLTFModel : public Model
{
public:
	GLTFModel() = default;
	GLTFModel( vk::Device* device, const std::filesystem::path& filePath );
	~GLTFModel() override = default;

	void Draw( const vk::DrawInfo& drawInfo ) override;
	void LoadTextures( vk::TextureManager& textureManager, const std::vector<std::string>& textureNames ) override;
private:
	//helpers
	template<typename T> //either uint32_t or uint16_t
	void CalculateTangentBitangent( std::vector<Vertex>& vertices, const std::vector<T>& indices );
	void LoadMeshes( fastgltf::Asset& asset, std::vector<Vertex>& vertexBuffer, std::vector<uint32_t>& indexBuffer );
	std::string LoadImage( const fastgltf::Image& image );
private:
	VkIndexType m_indexBufferType = VK_INDEX_TYPE_UINT32;
	std::vector<std::shared_ptr<vk::Texture>> m_textures;
};

#endif

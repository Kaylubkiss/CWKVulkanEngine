#pragma once
//NOTE (11/5/25): JUST SUPPORTING MESH AND TEXTURES FOR NOW
#include "IModel.h"

class GLTFModel : public IModel
{
public:
	GLTFModel() = default;
	~GLTFModel() = default;
	void LoadObject( vk::Device* device );
	virtual void UpdateModelTransform(const glm::mat4& newModelMatrix) { }; //no physics for GLTF yet.
	void Draw( VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout );
private:
	void LoadGLTF();
	//helpers
	void LoadMesh( fastgltf::Mesh& mesh, std::vector<Vertex>& vertexBuffer, std::vector<uint16_t>& indexBuffer );
	void LoadImage( vk::Device* devicePtr, fastgltf::Image& image );
private:
	fastgltf::Asset m_asset; //TODO: move this out because it's too big.
	std::vector<std::shared_ptr<vk::Texture>> m_textures;



};

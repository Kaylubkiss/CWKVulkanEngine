#ifndef MODEL_HPP
#define MODEL_HPP

#include "Node.h"

namespace vk
{
	class TextureManager;
	class DescriptorManager;
}

struct Primitive
{
	uint32_t firstIndex = 0;
	uint32_t indexCount = 0;

	uint32_t firstVertex = 0;
	uint32_t vertexCount = 0;

	uint32_t textureSetLayoutIndex = 0;

	std::optional<size_t> baseColorMaterialIndex;
	std::optional<size_t> normalMaterialIndex;
	std::optional<size_t> metallicRoughnessIndex;
	std::optional<size_t> ambientOcclusionIndex;
};

struct Mesh
{
	std::string m_name;
	std::vector<Primitive> m_primitives;
	Mesh() = default;
	Mesh( const std::string& name, const std::vector<Primitive>& primitives )
	{
		m_name = name;
		m_primitives = primitives;
	}
};

class Model
{
	friend class GLTFModel; //this is so animation channels can point to nodes. Wanted to keep read-only for everybody else.
public:
	virtual ~Model() = default;

	//get the bounds of the model in object space.
	[[nodiscard]] virtual glm::vec3 GetMinPoint() const;
	[[nodiscard]] virtual glm::vec3 GetMaxPoint() const;

	const std::vector<Node>& GetNodes();
	const std::vector<size_t>& GetSceneNodeIndices();
	const std::vector<std::shared_ptr<Mesh>>& GetMeshes();

	void AddMesh( const std::shared_ptr<Mesh>& mesh );
	void AddNode( const Node& node );
	void AddSceneNodeIndex(size_t nodeIndex);
	void AddTextureName(const std::string& textureName);

	[[nodiscard]] const glm::mat4& GetModelTransform() const;

	//for now, assume we only have one physics component for an entire hierarchy of meshes.\
	Obviously, there will need to be an overhaul with this. 
	void UpdateModelTransform( const glm::mat4& newModelMatrix );

	void DrawMeshPrimitives( const vk::DrawInfo& drawInfo, const std::vector<Primitive>& primitives );

	virtual void Draw( const vk::DrawInfo& drawInfo ) = 0;
	virtual void UpdateAnimation( const float deltaTime, const uint32_t currentFrameInFlight ) { (void)(deltaTime); (void)(currentFrameInFlight); }
	virtual void LoadTextures( vk::TextureManager& textureManager, const std::vector<std::string>& textureNames ) = 0;
	virtual void UploadSkinDescriptor( vk::DescriptorManager& descriptorManager ) { (void)(descriptorManager); }
	[[nodiscard]] const std::vector<std::string>& GetTextureNames() const { return m_textureNames; }
protected:
	vk::Buffer& GetVertexBuffer();
	vk::Buffer& GetIndexBuffer();
private:
	glm::mat4 m_modelTransform  = glm::mat4(1.f);
	std::vector<Node> m_nodes;
	std::vector<size_t> m_sceneNodeIndices;
	std::vector<std::string> m_textureNames;
	std::vector<std::shared_ptr<Mesh>> m_meshes;
	vk::Buffer m_vertexBuffer;
	vk::Buffer m_indexBuffer;
};

#endif
#pragma once
class TextureManager;

struct Node //notice: no pointing to parents. Might be something for later.
{
	glm::mat4 transform;
	std::optional<size_t> meshIndex;
	std::vector<size_t> childrenIndices;
	std::string name;
};

struct Primitive
{
	uint32_t firstIndex = 0;
	uint32_t indexCount = 0;

	uint32_t firstVertex = 0;
	uint32_t vertexCount = 0;

	uint32_t textureSetLayoutIndex = 0;

	std::optional<size_t> baseColorMaterialIndex;
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
public:
	virtual ~Model() = default;

	//get the bounds of the model in object space.
	[[nodiscard]] virtual glm::vec3 GetMinPoint() const { return { 0.0f, 0.0f, 0.0f }; }
	[[nodiscard]] virtual glm::vec3 GetMaxPoint() const {return { 0.0f, 0.0f, 0.0f }; }

	const std::vector<Node>& GetNodes()
	{
		return m_nodes;
	}

	const std::vector<size_t>& GetSceneNodeIndices()
	{
		return m_sceneNodeIndices;
	}

	const std::vector<std::shared_ptr<Mesh>>& GetMeshes()
	{
		return m_meshes;
	}

	void AddMesh( const std::shared_ptr<Mesh>& mesh )
	{
		m_meshes.push_back(mesh);
	}

	void AddNode( const Node& node )
	{
		m_nodes.push_back(node);
	}

	void AddSceneNodeIndex(size_t nodeIndex)
	{
		m_sceneNodeIndices.push_back(nodeIndex);
	}

	void AddTextureName(const std::string& textureName)
	{
		m_textureNames.push_back(textureName);
	}

	[[nodiscard]] const glm::mat4& GetModelTransform() const { return m_modelTransform; }

	//for now, assume we only have one physics component for an entire hierarchy of meshes.\
	Obviously, there will need to be an overhaul with this. 
	void UpdateModelTransform( const glm::mat4& newModelMatrix )
	{
		m_modelTransform = newModelMatrix;
	}

	virtual void Draw( const vk::DrawInfo& drawInfo ) = 0;
	virtual void LoadTextures( TextureManager& textureManager, const std::vector<std::string>& textureNames ) = 0;
	[[nodiscard]] const std::vector<std::string>& GetTextureNames() const { return m_textureNames; }
	static void DrawMeshPrimitives( const vk::DrawInfo& drawInfo, const std::vector<Primitive>& primitives )
	{
		for (auto& primitive : primitives)
		{
			VkDeviceSize descriptorBufferOffset =
				primitive.textureSetLayoutIndex * drawInfo.textureBindingSize;

			g_vkCmdSetDescriptorBufferOffsetsEXT(drawInfo.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
					   drawInfo.pipelineLayout, drawInfo.firstSet, drawInfo.setCount,
					   &drawInfo.imageBufferIndex, &descriptorBufferOffset);

			vkCmdDrawIndexed(drawInfo.cmdBuffer, primitive.indexCount, 1,
				primitive.firstIndex, primitive.firstVertex, 0); //indexing into 1 vertex buffer.
		}
	}
protected:
	vk::Buffer& GetVertexBuffer()
	{
		return m_vertexBuffer;
	}
	vk::Buffer& GetIndexBuffer()
	{
		return m_indexBuffer;
	}
private:
	glm::mat4 m_modelTransform  = glm::mat4(1.f);
	std::vector<Node> m_nodes;
	std::vector<size_t> m_sceneNodeIndices;
	std::vector<std::string> m_textureNames;
	std::vector<std::shared_ptr<Mesh>> m_meshes;
	vk::Buffer m_vertexBuffer;
	vk::Buffer m_indexBuffer;
};
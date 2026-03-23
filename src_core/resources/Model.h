#pragma once
class TextureManager;
class ResourceManager;

struct Node //notice: no pointing to parents. Might be something for later.
{
	glm::mat4 transform;
	std::optional<size_t> meshIndex;
	std::vector<size_t> childrenIndices;
};

class Model : public Resource {
public:
	~Model() override
	{
		m_vertexBuffer.Destroy();
		m_indexBuffer.Destroy();
	}

	//for now, assume we only have one physics component for an entire hierarchy of meshes.\
	Obviously, there will need to be an overhaul with this. 
	virtual void UpdateModelTransform( const glm::mat4& newModelMatrix ) = 0;
	virtual void Draw( const vk::DrawInfo& drawInfo ) = 0;
	virtual void LoadTextures( TextureManager& textureManager, const std::vector<std::string>& textureNames ) = 0;

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

	static void DrawMeshPrimitives( const vk::DrawInfo& drawInfo, const std::vector<Primitive>& primitives )
	{
		for (auto& primitive : primitives)
		{
			VkDeviceSize descriptorBufferOffset =
				primitive.textureSetLayoutIndex * drawInfo.textureLayoutSize;

			g_vkCmdSetDescriptorBufferOffsetsEXT(drawInfo.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
					   drawInfo.pipelineLayout, drawInfo.firstSet, drawInfo.setCount,
					   &drawInfo.imageBufferIndex, &descriptorBufferOffset);

			vkCmdDrawIndexed(drawInfo.cmdBuffer, primitive.indexCount, 1,
				primitive.firstIndex, 0, 0); //indexing into 1 vertex buffer.
		}
	}
protected:
	bool doLoad( vk::Device* devicePtr, ResourceManager& resourceManager ) override
	{
		(void)(resourceManager);
		(void)(devicePtr); return false;
	}
	void doUnload( vk::Device* devicePtr ) override { (void)(devicePtr); }
	vk::Buffer& GetVertexBuffer()
	{
		return m_vertexBuffer;
	}
	vk::Buffer& GetIndexBuffer()
	{
		return m_indexBuffer;
	}
private:
	std::vector<Node> m_nodes;
	std::vector<size_t> m_sceneNodeIndices;
	std::vector<std::shared_ptr<Mesh>> m_meshes;
	//TODO: make a generic buffer object?
	vk::Buffer m_vertexBuffer;
	vk::Buffer m_indexBuffer;
};
#pragma once

//NOTE (11/5/25): JUST SUPPORTING MESH AND TEXTURES FOR NOW

//each element is based on the gltf spec. Nodes for example contain info that is seen in glTF nodes. Hence, the name of the namespace.

namespace vkGltf 
{
	struct Primitive {
		uint32_t firstIndex = 0;
		uint32_t indexCount = 0;
		
		uint32_t firstVertex = 0;
		uint32_t vertexCount = 0;
	};


	struct Mesh 
	{
		std::string name = "";

		std::vector<Primitive> primitives;
	};

	//WIP -- do when GLTF is fully functional
	/*struct Node 
	{
		std::shared_ptr<Node> parent;
		std::vector<std::shared_ptr<Node>> children;

		glm::mat4 localMatrix = glm::mat4(1.f);

		glm::mat4 GetMatrix();
	};*/
	

	class Model
	{
		private:
			vk::Device* devicePtr = nullptr;

			fastgltf::Asset m_asset; //TODO: move this out because it's too big.

			vk::Buffer m_vertexBuffer;
			vk::Buffer m_indexBuffer;


			std::vector<std::shared_ptr<Mesh>> m_meshes;

			std::vector<std::shared_ptr<vk::Texture>> m_textures;

		public:
			Model() = default;
			~Model() = default;
			void Destroy();
			void LoadObject(vk::Device* device);

			void Draw(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout);

		private:
			void LoadGLTF();

			void LoadMesh(fastgltf::Mesh& mesh, 
				std::vector<Vertex>& vertexBuffer,
				std::vector<uint16_t>& indexBuffer);

			void LoadImage(fastgltf::Image& image);
		
	};

}


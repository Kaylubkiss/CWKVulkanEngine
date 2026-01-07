#include "GLTFLoading.h"

#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>
#include <variant>

#define GLTF_OBJECT_PATH "External/objects/gltf/"

namespace vkGltf
{
	/*glm::mat4 Node::GetMatrix() 
	{
		std::shared_ptr<Node> parent = this->parent;

		glm::mat4 matrix = this->localMatrix;

		while (parent.get()) 
		{
			matrix = parent.get()->localMatrix * matrix;

			parent = parent.get()->parent;
		}

		return matrix;
	}*/

	void Model::Destroy()
	{
		m_vertexBuffer.Destroy();
		m_indexBuffer.Destroy();
	}

	void Model::LoadObject(vk::Device* device)
	{
		assert(device != nullptr);

		this->devicePtr = device;

		LoadGLTF();

		std::vector<Vertex> vertices;
		std::vector<uint16_t> indices;

		for (auto& mesh : m_asset.meshes) {

			LoadMesh(mesh, vertices, indices);
		}

		for (auto& image : m_asset.images) 
		{
			LoadImage(image);
		}

		size_t vertexBufferSize = vertices.size() * sizeof(vertices[0]);
		size_t indexBufferSize = indices.size() * sizeof(indices[0]);

		vk::Buffer vertexStagingBuffer = device->CreateBuffer(
			vertexBufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			vertices.data());

		vk::Buffer indexStagingBuffer = device->CreateBuffer(
			indexBufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			indices.data());

		m_vertexBuffer = device->CreateBuffer(
			vertexBufferSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			nullptr);

		m_indexBuffer = device->CreateBuffer(
			indexBufferSize,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			nullptr);


		VkCommandBuffer copyCmd = device->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

		VkBufferCopy copyRegion = {};

		copyRegion.size = vertexBufferSize;
		vkCmdCopyBuffer(copyCmd, vertexStagingBuffer.handle, m_vertexBuffer.handle, 1, &copyRegion);

		copyRegion.size = indexBufferSize;
		vkCmdCopyBuffer(copyCmd, indexStagingBuffer.handle, m_indexBuffer.handle, 1, &copyRegion);

		device->FlushCommandBuffer(copyCmd, device->graphicsQueue.handle, device->commandPool, true);

		vertexStagingBuffer.Destroy();
		indexStagingBuffer.Destroy();
	}

	void Model::Draw(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout)
	{

		size_t sceneIndex = m_asset.defaultScene.value_or(0);

		const VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &m_vertexBuffer.handle, offsets);
		//TODO: assuming unsigned short for now, will have to change the way primitives perceive this.
		vkCmdBindIndexBuffer(cmdBuffer, m_indexBuffer.handle, 0, VK_INDEX_TYPE_UINT16);

		if (!m_asset.scenes.empty())
		{
			fastgltf::iterateSceneNodes(m_asset, sceneIndex, fastgltf::math::fmat4x4(),
				[&](fastgltf::Node& node, fastgltf::math::fmat4x4 matrix)
				{
					if (node.meshIndex.has_value())
					{
						auto mesh = m_meshes[node.meshIndex.value()];

						if (pipelineLayout != VK_NULL_HANDLE)
						{
							vkCmdPushConstants(cmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
								sizeof(matrix), matrix.data());
						}

						for (auto& primitive : mesh.get()->primitives)
						{
							//TODO: support textures..
							vkCmdDrawIndexed(cmdBuffer, primitive.indexCount, 1, primitive.firstIndex, primitive.firstVertex, 0);
						}
					}

				});
		}
	}

	void Model::LoadGLTF()
	{
		std::filesystem::path path = OBJECT_PATH + std::string("gltf/AnimatedCube.gltf");

		fastgltf::Parser parser;

		fastgltf::Expected<fastgltf::Asset> asset(fastgltf::Error::None);

		if (!std::filesystem::exists(path))
		{
			std::cout << "path does not exist\n";
			throw std::runtime_error("LoadObject() failed");
		}
		else
		{
			/*std::cout << "loading " << path << std::endl;*/
		}

		fastgltf::Options gltfOptions =
			fastgltf::Options::DontRequireValidAssetMember |
			fastgltf::Options::AllowDouble |
			fastgltf::Options::GenerateMeshIndices |
			fastgltf::Options::LoadExternalBuffers;

		fastgltf::GltfFileStream data(path);

		asset = parser.loadGltf(data, path.parent_path(), gltfOptions);

		if (asset.error() != fastgltf::Error::None)
		{
			std::cerr << "Couldn't load in specified data\n";
			throw std::runtime_error("LoadObject() failed");
		}

		m_asset = std::move(asset.get());

		std::cout << "successfully loaded " << path << std::endl;

	}

	void Model::LoadMesh(fastgltf::Mesh& mesh, std::vector<Vertex>& vertexBuffer,
	std::vector<uint16_t>& indexBuffer)
	{
		std::shared_ptr<vkGltf::Mesh> newMesh = std::make_shared<Mesh>();

		newMesh.get()->name = mesh.name;

		for (auto& primitive : mesh.primitives)
		{
			Primitive newPrim = {};
			newPrim.firstIndex = static_cast<uint32_t>(indexBuffer.size());
			newPrim.firstVertex = static_cast<uint32_t>(vertexBuffer.size());
			newPrim.indexCount = 0;
			newPrim.vertexCount = 0;

			//vertex
			{
				//positions
				auto positionAttrib = primitive.findAttribute("POSITION");
				if (positionAttrib != primitive.attributes.end())
				{
					fastgltf::Accessor& posAccessor = m_asset.accessors[positionAttrib->accessorIndex];

					newPrim.vertexCount = static_cast<uint32_t>(m_asset.accessors[positionAttrib->accessorIndex].count);

					vertexBuffer.resize(newPrim.vertexCount + static_cast<uint32_t>(vertexBuffer.size()));

					//this is possible with Options::LoadExternalBuffers
					fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(m_asset, posAccessor,
					[&](fastgltf::math::fvec3 pos, size_t idx)
					{
						vertexBuffer[newPrim.firstVertex + idx].pos = glm::make_vec3(pos.data());
						vertexBuffer[newPrim.firstVertex + idx].nrm = glm::vec3(0);
						vertexBuffer[newPrim.firstVertex + idx].uv = glm::vec2(0);
					});
				}
				else
				{
					throw std::runtime_error("Primitive should have position attributes\n");
				}

				auto normAttrib = primitive.findAttribute("NORMAL");
				if (normAttrib != primitive.attributes.end())
				{
					fastgltf::Accessor& normalAccessor = m_asset.accessors[normAttrib->accessorIndex];
					fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(m_asset, normalAccessor,
					[&](fastgltf::math::fvec3 norm, size_t idx)
					{
						vertexBuffer[newPrim.firstVertex + idx].nrm = glm::make_vec3(norm.data());
					});

				}

				auto texCoordAttrib = primitive.findAttribute("TEXCOORD_0");
				if (texCoordAttrib != primitive.attributes.end())
				{
					fastgltf::Accessor& texCoordAccessor = m_asset.accessors[texCoordAttrib->accessorIndex];
					fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(m_asset, texCoordAccessor,
					[&](fastgltf::math::fvec2 uv, size_t idx)
					{
						vertexBuffer[newPrim.firstVertex + idx].uv = glm::make_vec2(uv.data());
					});
				}

			}
			//end of vertex

			//indices
			{
				fastgltf::Accessor& accessor = m_asset.accessors[primitive.indicesAccessor.value()];
				if (!accessor.bufferViewIndex.has_value())
				{
					throw std::runtime_error("gltf asset should have an index buffer\n");
				}

				newPrim.indexCount = static_cast<uint32_t>(accessor.count);

				if ((accessor.componentType == fastgltf::ComponentType::UnsignedByte) ||
					(accessor.componentType == fastgltf::ComponentType::UnsignedShort))
				{
					std::vector<uint16_t> buf(newPrim.indexCount);
					fastgltf::copyFromAccessor<uint16_t>(m_asset, accessor, buf.data());

					for (auto& index : buf)
					{
						indexBuffer.push_back(newPrim.firstVertex + index);
					}
				}
				else //unsigned int
				{
					std::vector<uint32_t> buf(newPrim.indexCount);
					fastgltf::copyFromAccessor<uint32_t>(m_asset, accessor, buf.data());

					for (auto& index : buf)
					{
						indexBuffer.push_back(newPrim.firstVertex + index);
					}
				}

			}
			//end of indices

			newMesh.get()->primitives.push_back(newPrim);
		}

		m_meshes.push_back(newMesh);
	}


	void Model::LoadImage(fastgltf::Image& image) 
	{
		std::visit(fastgltf::visitor{
			[](auto& arg) {},
			[&](fastgltf::sources::URI& filePath) {
				assert(filePath.fileByteOffset == 0);

				m_textures.emplace_back(std::make_shared<vk::Texture>(devicePtr, std::string(filePath.uri.c_str())));
			},
		}, image.data);


		//TODO if texture ends up empty, assign it a default (checkerboard) texture.


	}
}

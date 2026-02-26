#include "GLTFModel.h"
#include <glm/gtc/type_ptr.hpp>
#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>
#include <variant>



GLTFModel::GLTFModel( vk::Device* device, const std::filesystem::path& filePath )
{
	fastgltf::Options gltfOptions =
		fastgltf::Options::DontRequireValidAssetMember |
		fastgltf::Options::AllowDouble |
		fastgltf::Options::GenerateMeshIndices |
		fastgltf::Options::LoadExternalBuffers;

	fastgltf::GltfFileStream data(filePath);

	fastgltf::Expected<fastgltf::Asset> asset(fastgltf::Error::None);
	fastgltf::Parser parser;

	asset = parser.loadGltf(data, filePath.parent_path(), gltfOptions);

	if (asset.error() != fastgltf::Error::None)
	{
		std::cerr << "Couldn't load in specified data\n";
		throw std::runtime_error("LoadObject() failed");
	}

	m_asset = std::move(asset.get());

	std::cout << "successfully loaded " << filePath.string() << std::endl;

	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;

	for (auto& mesh : m_asset.meshes)
	{
		LoadMesh(mesh, vertices, indices);
	}

	std::vector<std::string> fileNames(m_asset.images.size());
	for (auto& image : m_asset.images)
	{
		fileNames.push_back(LoadImage(device, image));
	}


	size_t vertexBufferSize = vertices.size() * sizeof(vertices[0]);
	size_t indexBufferSize = indices.size() * sizeof(indices[0]);

	m_vertexBuffer = device->CreateBuffer(
		vertexBufferSize,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		vertices.data());

	m_indexBuffer = device->CreateBuffer(
		indexBufferSize,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT ,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		indices.data());
}

std::vector<std::string> GLTFModel::GetTextureFileNames() const
{
	std::vector<std::string> fileNames;
	for (auto& image : m_asset.images)
	{
		std::visit(fastgltf::visitor
		{
			[](auto& arg) {},
			[&](const fastgltf::sources::URI& filePath) {
				assert(filePath.fileByteOffset == 0);
				fileNames.emplace_back(filePath.uri.path());
			},
		}, image.data);
	}

	return fileNames;
}

void GLTFModel::UpdateModelTransform( const glm::mat4& newModelMatrix )
{
	m_modelMatrix = newModelMatrix;
}

void GLTFModel::Draw( const vk::DrawInfo& drawInfo )
{
	const size_t sceneIndex = m_asset.defaultScene.value_or(0);

	constexpr VkDeviceSize offsets[1] = { 0 };

	VkBuffer vertexBuffer = m_vertexBuffer.GetHandle();
	VkBuffer indexBuffer  = m_indexBuffer.GetHandle();

	vkCmdBindVertexBuffers(drawInfo.cmdBuffer, 0, 1, &vertexBuffer, offsets);
	//TODO: assuming unsigned short for now, will have to change the way primitives perceive this.
	vkCmdBindIndexBuffer(drawInfo.cmdBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);


	if (!m_asset.scenes.empty())
	{
		//TODO: inefficient copy
		//Ideally, this codebase will use one math library as its basis.
		fastgltf::math::fmat4x4 modelMatrix;
		for (size_t i = 0; i < modelMatrix.rows(); ++i)
		{
			for (size_t j = 0; j < modelMatrix.columns(); ++j)
			{
				modelMatrix[i][j] = m_modelMatrix[i][j];
			}
		}

		fastgltf::iterateSceneNodes(m_asset, sceneIndex, fastgltf::math::fmat4x4(),
			[&](fastgltf::Node& node, fastgltf::math::fmat4x4 matrix)
		{
			if (node.meshIndex.has_value())
			{
				matrix = modelMatrix * matrix;

				auto mesh = m_meshes[node.meshIndex.value()];

				if (drawInfo.pipelineLayout != VK_NULL_HANDLE)
				{
					vkCmdPushConstants(drawInfo.cmdBuffer, drawInfo.pipelineLayout,
						VK_SHADER_STAGE_VERTEX_BIT, 0,
						sizeof(matrix), matrix.data());
				}

				DrawMeshPrimitives(drawInfo, mesh->m_primitives);
			}
		});
	}
}

void GLTFModel::LoadTextures( TextureManager& textureManager, const std::vector<std::string>& textureNames )
{
	//TODO
	for (auto& mesh: m_meshes)
	{
		for (auto& primitive : mesh->m_primitives)
		{
			//TODO: this < 2 is TEMPORARY. The shaders only support up to 2 bindings
			for (size_t i = 0; i < textureNames.size() && i < 2; ++i)
			{
				textureManager.BindTextureToModelPrimitive(
					textureNames[i],
					static_cast<uint32_t>(i),
					primitive.textureSetLayoutIndex);
			}
		}
	}

}

void GLTFModel::LoadMesh( fastgltf::Mesh& mesh, std::vector<Vertex>& vertexBuffer,
	std::vector<uint16_t>& indexBuffer )
{
	std::shared_ptr<Mesh> newMesh = std::make_shared<Mesh>();

	newMesh->m_name = mesh.name;

	for (auto& primitive : mesh.primitives)
	{
		Primitive newPrim   = {};
		newPrim.firstIndex  = static_cast<uint32_t>(indexBuffer.size());
		newPrim.firstVertex = static_cast<uint32_t>(vertexBuffer.size());
		newPrim.indexCount  = 0;
		newPrim.vertexCount = 0;
		uint32_t materialIndex = static_cast<uint32_t>(primitive.materialIndex.value_or(0));
		/*fastgltf::Material& material = m_asset.materials[materialIndex];
		fastgltf::PBRData& pbr = material.pbrData;*/

		//TODO: support normal texture, occlusion, emissive with accompanying parameters in fastgltf::Material

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
						vertexBuffer[newPrim.firstVertex + idx].uv  = glm::vec2(0);
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

			auto texCoordAttrib = primitive.findAttribute("TEXCOORD_" + std::to_string(0));
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
			if (accessor.bufferViewIndex.has_value() == false)
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

		newMesh->m_primitives.push_back(newPrim);
	}

	m_meshes.push_back(newMesh);
}

std::string GLTFModel::LoadImage( vk::Device* devicePtr, fastgltf::Image& image )
{
	std::visit(fastgltf::visitor
	{
		[](auto& arg) {},
		[&](const fastgltf::sources::URI& filePath) {
			assert(filePath.fileByteOffset == 0);

			return filePath.uri.string();
		},
	}, image.data);

	return "";
}


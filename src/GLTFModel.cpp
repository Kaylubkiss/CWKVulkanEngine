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

	fastgltf::Expected<fastgltf::Asset> expected_asset(fastgltf::Error::None);
	fastgltf::Asset asset;
	fastgltf::Parser parser;

	expected_asset = parser.loadGltf(data, filePath.parent_path(), gltfOptions);

	if (expected_asset.error() != fastgltf::Error::None) {
		std::cerr << "Couldn't load in specified data\n";
		throw std::runtime_error("LoadObject() failed");
	}

	asset = std::move(expected_asset.get());

	if (fastgltf::validate(asset) != fastgltf::Error::None)
	{
		std::cerr << "object is an invalid gltf file!\n";
		throw std::runtime_error("LoadObject() failed");
	}

	std::cout << "successfully loaded " << filePath.string() << std::endl;

	std::vector<Vertex> vertices;
	//much safer to assume that values will lie in a larger range, can always
	//downcast otherwise.
	std::vector<uint32_t> indices_32;

	LoadMeshes(asset, vertices, indices_32);

	auto& scene = asset.scenes[0];
	for (auto nodeIndex : scene.nodeIndices)
	{
		AddSceneNodeIndex(nodeIndex);
	}

	for (auto& image : asset.images)
	{
		std::string texture_name = LoadImage(image);
		if (texture_name.empty() == false)
		{
			AddTextureName(texture_name);
		}
	}

	for (auto& node : asset.nodes)
	{
		Node newNode = {};

		newNode.name = node.name;
		newNode.meshIndex = node.meshIndex;

		for (auto& child : node.children)
		{
			newNode.childrenIndices.push_back(child);
		}

		if (std::holds_alternative<fastgltf::math::fmat4x4>(node.transform) == true)
		{
			memcpy(&newNode.transform[0], std::get<fastgltf::math::fmat4x4>(node.transform).data(),
				sizeof(newNode.transform));
		}
		else
		{

			fastgltf::TRS trs = std::get<fastgltf::TRS>(node.transform);


			fastgltf::math::fmat4x4 transform = fastgltf::math::fmat4x4(1.0);
			transform = fastgltf::math::scale(transform, trs.scale);
			transform = fastgltf::math::rotate(transform, trs.rotation);
			transform = fastgltf::math::translate(transform, trs.translation);

			memcpy(&newNode.transform[0], transform.data(),
				sizeof(newNode.transform));
		}

		AddNode(newNode);
	}

	size_t vertexBufferSize = vertices.size() * sizeof(vertices[0]);
	size_t indexBufferSize = indices_32.size() * sizeof(indices_32[0]);
	void* indicesData = indices_32.data();

	std::vector<uint16_t> indices_16;
	if (m_indexBufferType == VK_INDEX_TYPE_UINT16)
	{
		indices_16.resize(indices_32.size());
		for (size_t i = 0; i < indices_16.size(); ++i)
		{
			indices_16[i] = static_cast<uint16_t>(indices_32[i]);
		}

		indexBufferSize = indices_16.size() * sizeof(indices_16[0]);
		indicesData = indices_16.data();
	}

	auto& vertexBuffer = GetVertexBuffer();
	auto& indexBuffer = GetIndexBuffer();

	vertexBuffer = device->CreateBuffer(
		vertexBufferSize,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		vertices.data());

	indexBuffer = device->CreateBuffer(
		indexBufferSize,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT ,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		indicesData);
}

void GLTFModel::Draw( const vk::DrawInfo& drawInfo )
{
	constexpr VkDeviceSize offsets[1] = { 0 };

	auto& vertexBuffer = GetVertexBuffer();
	auto& indexBuffer  = GetIndexBuffer();
	VkBuffer vertexBufferHandle = vertexBuffer.GetHandle();

	vkCmdBindVertexBuffers(drawInfo.cmdBuffer, 0, 1, &vertexBufferHandle, offsets);

	//TODO: assuming unsigned short for now, will have to change the way primitives perceive this.
	vkCmdBindIndexBuffer(drawInfo.cmdBuffer, indexBuffer.GetHandle(), 0, m_indexBufferType);

	auto& nodeIndices = GetSceneNodeIndices();
	auto& nodes = GetNodes();
	auto& meshes = GetMeshes();
	auto& modelTransform = GetModelTransform();

	std::function<void(size_t, glm::mat4)> func = [&](size_t nodeIndex, glm::mat4 parentTransform)
	{
		assert(nodeIndex < nodes.size());

		const Node& node = nodes[nodeIndex];

		glm::mat4 curr_transform = parentTransform * node.transform;

		if (node.meshIndex.has_value())
		{
			//do rendering here.

			auto mesh = meshes[node.meshIndex.value()];

			if (drawInfo.pipelineLayout != VK_NULL_HANDLE)
			{
				vkCmdPushConstants(drawInfo.cmdBuffer, drawInfo.pipelineLayout,
					VK_SHADER_STAGE_VERTEX_BIT, 0,
					sizeof(curr_transform), &curr_transform[0]);
			}

			DrawMeshPrimitives(drawInfo, mesh->m_primitives);
		}

		for (auto& childIndex : node.childrenIndices)
		{
			func(childIndex, curr_transform);
		}
	};

	for (auto& sceneNode : nodeIndices)
	{
		func(sceneNode, modelTransform);
	}
}

void GLTFModel::LoadTextures( TextureManager& textureManager, const std::vector<std::string>& textureNames )
{
	if (textureNames.empty())
	{
		return;
	}

	auto& meshes = GetMeshes();

	for (size_t i = 0; i < meshes.size(); ++i)
	{
		for (size_t j = 0; j < meshes[i]->m_primitives.size(); ++j)
		{
			Primitive& primitive = meshes[i]->m_primitives[j];

			std::vector<std::string> primitive_textureNames;

			if (primitive.baseColorMaterialIndex.has_value())
			{
				auto baseColorIndex = primitive.baseColorMaterialIndex.value();

				primitive_textureNames.push_back(textureNames[baseColorIndex]);
				//grab all the texture names specific to this primitive and then request the texture manager to make a
				//layout with the bindings starting from index 0 -> n, where n is the number of textures.
			}

			if (primitive.metallicRoughnessIndex.has_value())
			{
				auto mrIndex = primitive.metallicRoughnessIndex.value();

				primitive_textureNames.push_back(textureNames[mrIndex]);
			}

			if (primitive.ambientOcclusionIndex.has_value())
			{
				auto aoIndex = primitive.ambientOcclusionIndex.value();

				primitive_textureNames.push_back(textureNames[aoIndex]);
			}

			primitive.textureSetLayoutIndex = textureManager.AddTextures(primitive_textureNames);
		}
	}

}

void GLTFModel::LoadMeshes( fastgltf::Asset& asset, std::vector<Vertex>& vertexBuffer,
	std::vector<uint32_t>& indexBuffer )
{

	for (size_t i = 0; i < asset.meshes.size(); ++i)
	{
		fastgltf::Mesh& mesh = asset.meshes[i];

		std::shared_ptr<Mesh> newMesh = std::make_shared<Mesh>();

		newMesh->m_name = mesh.name;



		for (auto& primitive : mesh.primitives)
		{
			if (primitive.type != fastgltf::PrimitiveType::Triangles)
			{
				std::cerr << "primitive must be triangulated!\n";
				throw std::runtime_error("GLTFModel::LoadMeshes() Failed!\n");
			}

			Primitive newPrim   = {};
			newPrim.firstIndex  = static_cast<uint32_t>(indexBuffer.size());
			newPrim.firstVertex = static_cast<uint32_t>(vertexBuffer.size());
			newPrim.indexCount  = 0;
			newPrim.vertexCount = 0;
			fastgltf::Material& p_material = asset.materials[primitive.materialIndex.value()];
			fastgltf::PBRData& p_pbr = p_material.pbrData;

			if (p_pbr.baseColorTexture.has_value())
			{
				newPrim.baseColorMaterialIndex = p_pbr.baseColorTexture.value().textureIndex;
			}

			if (p_pbr.metallicRoughnessTexture.has_value())
			{
				newPrim.metallicRoughnessIndex = p_pbr.metallicRoughnessTexture.value().textureIndex;
			}

			if (p_material.occlusionTexture.has_value())
			{
				newPrim.ambientOcclusionIndex = p_material.occlusionTexture.value().textureIndex;
			}

			//vertex
			{
				//positions
				auto positionAttrib = primitive.findAttribute("POSITION");
				if (positionAttrib != primitive.attributes.end())
				{
					fastgltf::Accessor& posAccessor = asset.accessors[positionAttrib->accessorIndex];

					newPrim.vertexCount = static_cast<uint32_t>(posAccessor.count);

					vertexBuffer.resize(newPrim.vertexCount + newPrim.firstVertex);

					//this is possible with Options::LoadExternalBuffers
					fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset, posAccessor,
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
					fastgltf::Accessor& normalAccessor = asset.accessors[normAttrib->accessorIndex];
					fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset, normalAccessor,
						[&](fastgltf::math::fvec3 norm, size_t idx)
						{
							vertexBuffer[newPrim.firstVertex + idx].nrm = glm::make_vec3(norm.data());
						});

				}

				//NOTE: might be multiple texcoord_
				//texcoord_0 usually denotes the base color, however.
				auto texCoordAttrib = primitive.findAttribute("TEXCOORD_" + std::to_string(0));
				if (texCoordAttrib != primitive.attributes.end())
				{
					fastgltf::Accessor& texCoordAccessor = asset.accessors[texCoordAttrib->accessorIndex];
					fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(asset, texCoordAccessor,
						[&](fastgltf::math::fvec2 uv, size_t idx)
						{
							vertexBuffer[newPrim.firstVertex + idx].uv = glm::make_vec2(uv.data());
						});
				}
			}
			//end of vertex

			//indices
			{
				fastgltf::Accessor& accessor = asset.accessors[primitive.indicesAccessor.value()];
				if (accessor.bufferViewIndex.has_value() == false)
				{
					throw std::runtime_error("gltf asset should have an index buffer\n");
				}

				newPrim.indexCount = static_cast<uint32_t>(accessor.count);

				if ((accessor.componentType == fastgltf::ComponentType::UnsignedByte) ||
					(accessor.componentType == fastgltf::ComponentType::UnsignedShort))
				{
					std::vector<uint16_t> buf(newPrim.indexCount);
					fastgltf::copyFromAccessor<uint16_t>(asset, accessor, buf.data());

					m_indexBufferType = VK_INDEX_TYPE_UINT16;

					for (auto& index : buf)
					{
						indexBuffer.push_back(index);
					}
				}
				else if (accessor.componentType == fastgltf::ComponentType::UnsignedInt)
				{
					std::vector<uint32_t> buf(newPrim.indexCount);
					fastgltf::copyFromAccessor<uint32_t>(asset, accessor, buf.data());

					for (auto& index : buf)
					{
						indexBuffer.push_back(index);
					}
				}
				else
				{
					std::cerr << "gltf asset is using unspecified component type for indices\n";
					throw std::runtime_error("GLTFModel::LoadMeshes() Failed!\n");
				}
			}
			//end of indices

			newMesh->m_primitives.push_back(newPrim);
		}

		AddMesh(newMesh);
	}
}

std::string GLTFModel::LoadImage(const fastgltf::Image& image )
{
	std::string fileName;

	std::visit(fastgltf::visitor
	{
		[](auto& arg) {},
		[&](const fastgltf::sources::URI& filePath) {
			assert(filePath.fileByteOffset == 0);
			fileName = filePath.uri.string();
		},
	}, image.data);

	return fileName;
}


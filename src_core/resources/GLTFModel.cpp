#include "GLTFModel.h"
#include <glm/gtc/type_ptr.hpp>
#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>
#include <variant>

//this is to ensure that we can memcpy, as the UpdateModelTransform() takes in a glm::mat4.
static_assert(sizeof(fastgltf::math::fmat4x4) == sizeof(glm::mat4));

bool GLTFModel::doLoad( vk::Device* devicePtr, ResourceManager& resourceManager )
{
	std::vector<Vertex> vertices;
	//much safer to assume that values will lie in a larger range, can always
	//downcast otherwise.
	std::vector<uint32_t> indices_32;
	fastgltf::Expected<fastgltf::Asset> expectedAsset(fastgltf::Error::None);
	fastgltf::Parser parser;

	fastgltf::Options gltfOptions =
	fastgltf::Options::DontRequireValidAssetMember |
	fastgltf::Options::AllowDouble |
	fastgltf::Options::GenerateMeshIndices |
	fastgltf::Options::LoadExternalBuffers;

	fastgltf::GltfFileStream data(GetId());

	expectedAsset = parser.loadGltf(data, GetId(), gltfOptions);

	if (expectedAsset.error() != fastgltf::Error::None)
	{
		std::cerr << "Couldn't load in specified data from " << GetId() << "\n";
		return false;
	}

	fastgltf::Asset& asset = expectedAsset.get();

	LoadMeshes(asset, vertices, indices_32);

	std::vector<std::string> textureNames;
	textureNames.resize(asset.images.size());

	for (auto& image : asset.images)
	{
		textureNames.push_back(LoadImage(devicePtr, image));
	}

	std::vector<ResourceHandle<vk::Texture>> textureHandles;
	textureHandles.reserve(textureNames.size());

	for (auto& texture : textureNames)
	{
		textureHandles.push_back(resourceManager.Load<vk::Texture>(texture));
	}

	//this is so we can correctly index into the *potentially* larger-sized node vector.
	auto& scene = asset.scenes[0];
	for (auto nodeIndex : scene.nodeIndices)
	{
		AddSceneNodeIndex(nodeIndex);
	}

	for (auto& node : asset.nodes)
	{
		Node newNode = {};

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
		else //TODO: extract the TRS components here
		{
			std::cerr << "mesh nodes only accept fmat4x4 for now!\n";
			throw std::runtime_error("GLTFModel::doLoad() Failed!\n");
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

	vertexBuffer = devicePtr->CreateBuffer(
		vertexBufferSize,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		vertices.data());

	indexBuffer = devicePtr->CreateBuffer(
		indexBufferSize,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT ,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		indicesData);

	return true;
}

void GLTFModel::UpdateModelTransform( const glm::mat4& newModelMatrix )
{
	m_modelMatrix = newModelMatrix;
}

void GLTFModel::Draw( const vk::DrawInfo& drawInfo )
{
	/*const size_t sceneIndex = m_asset.defaultScene.value_or(0);*/

	constexpr VkDeviceSize offsets[1] = { 0 };

	auto& vertexBuffer = GetVertexBuffer();
	auto& indexBuffer  = GetIndexBuffer();

	vkCmdBindVertexBuffers(drawInfo.cmdBuffer, 0, 1, &vertexBuffer.GetHandle(), offsets);

	//TODO: assuming unsigned short for now, will have to change the way primitives perceive this.
	vkCmdBindIndexBuffer(drawInfo.cmdBuffer, indexBuffer.GetHandle(), 0, m_indexBufferType);

	auto& nodeIndices = GetSceneNodeIndices();
	auto& nodes = GetNodes();
	auto& meshes = GetMeshes();
	std::function<void(size_t, glm::mat4)> func = [&](size_t nodeIndex, glm::mat4 parentTransform)
	{
		assert(nodeIndex < nodes.size());
		const Node& node = nodes[nodeIndex];

		glm::mat4 curr_transform = parentTransform * node.transform;

		//do rendering here.
		auto mesh = meshes[node.meshIndex.value()];

		if (drawInfo.pipelineLayout != VK_NULL_HANDLE)
		{
			vkCmdPushConstants(drawInfo.cmdBuffer, drawInfo.pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT, 0,
				sizeof(curr_transform), &curr_transform[0]);
		}

		DrawMeshPrimitives(drawInfo, mesh->m_primitives);

		for (auto& childIndex : node.childrenIndices)
		{
			func(childIndex, curr_transform);
		}
	};

	for (auto& sceneNode : nodeIndices)
	{
		func(sceneNode, m_modelMatrix);
	}

}

/*void GLTFModel::LoadTextures( TextureManager& textureManager, const std::vector<std::string>& textureNames )
{
	for (size_t i = 0; i < m_asset.meshes.size(); ++i)
	{
		fastgltf::Mesh& mesh = m_asset.meshes[i];

		for (size_t j = 0; j < mesh.primitives.size(); ++j)
		{
			fastgltf::Primitive& gltf_primitive = mesh.primitives[j];

			Primitive& vkc_primitive = m_meshes[i]->m_primitives[j];

			if (gltf_primitive.materialIndex.has_value())
			{
				fastgltf::Material& p_material = m_asset.materials[gltf_primitive.materialIndex.value()];
				fastgltf::PBRData& p_pbr = p_material.pbrData;

				uint32_t binding = 0;

				if (p_pbr.baseColorTexture.has_value())
				{
					size_t baseColorIndex = p_pbr.baseColorTexture.value().textureIndex;

					textureManager.AddTexture(textureNames[baseColorIndex], binding, vkc_primitive.textureSetLayoutIndex);

					++binding;
				}

				//I want to enforce that every texture layout must begin with a colored texture.
				//I want well-formed gltf files/assets.

				if (p_pbr.metallicRoughnessTexture.has_value())
				{
					assert(vkc_primitive.textureSetLayoutIndex > 0);

					size_t metallicRoughIndex = p_pbr.metallicRoughnessTexture.value().textureIndex;

					textureManager.AddTexture(textureNames[metallicRoughIndex], binding, vkc_primitive.textureSetLayoutIndex);

					++binding;
				}

				if (p_material.occlusionTexture.has_value())
				{
					assert(vkc_primitive.textureSetLayoutIndex > 0);

					size_t occlusionTextureIndex = p_material.occlusionTexture.value().textureIndex;

					textureManager.AddTexture(textureNames[occlusionTextureIndex], binding, vkc_primitive.textureSetLayoutIndex);

					++binding;
				}
			}
		}
	}

}*/

void GLTFModel::LoadMeshes( fastgltf::Asset& asset, std::vector<Vertex>& vertexBuffer,
	std::vector<uint32_t>& indexBuffer )
{

	for (auto& mesh : asset.meshes) {
		std::shared_ptr<Mesh> newMesh = std::make_shared<Mesh>();

		newMesh->m_name = mesh.name;

		for (auto& primitive : mesh.primitives)
		{
			Primitive newPrim   = {};
			newPrim.firstIndex  = static_cast<uint32_t>(indexBuffer.size());
			newPrim.firstVertex = static_cast<uint32_t>(vertexBuffer.size());
			newPrim.indexCount  = 0;
			newPrim.vertexCount = 0;

			//vertex
			{
				//positions
				auto positionAttrib = primitive.findAttribute("POSITION");
				if (positionAttrib != primitive.attributes.end())
				{
					fastgltf::Accessor& posAccessor = asset.accessors[positionAttrib->accessorIndex];

					newPrim.vertexCount = static_cast<uint32_t>(asset.accessors[positionAttrib->accessorIndex].count);

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

					for (auto& index : buf)
					{
						indexBuffer.push_back(newPrim.firstVertex + index);
					}
				}
				else //unsigned int
				{
					std::vector<uint32_t> buf(newPrim.indexCount);
					fastgltf::copyFromAccessor<uint32_t>(asset, accessor, buf.data());

					m_indexBufferType = VK_INDEX_TYPE_UINT32;

					for (auto& index : buf)
					{
						indexBuffer.push_back(newPrim.firstVertex + index);
					}
				}

			}
			//end of indices

			newMesh->m_primitives.push_back(newPrim);
		}

		AddMesh(newMesh);
	}
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


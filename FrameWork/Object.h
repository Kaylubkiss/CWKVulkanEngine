#pragma once
#include "Common.h"
#include <iostream>
#include <vector>
#include <string>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

enum MeshType
{
	M_NONE = 0,
	M_CUBE,
};

struct Vertex 
{
	glm::vec3 pos = {0,0,0};
	glm::vec3 nrm = {.2f,.5f,0};
	glm::vec2 uv = {-1.f,-1.f};

	bool operator==(const Vertex& other) const {
		return pos == other.pos && nrm == other.nrm && uv == other.uv;
	}
};

namespace std {
	template<> 
	struct hash<Vertex> {
		
		
			size_t operator()(Vertex const& vertex) const 
			{
				size_t h1 = hash<glm::vec3>{}(vertex.pos);
				size_t h2 = hash<glm::vec3>{}(vertex.nrm);
				size_t h3 = hash<glm::vec2>{}(vertex.uv);

				return ((h1 ^ (h2 << 1)) >> 1) ^ (h3 << 1);
			}
	};
}

struct Mesh
{
	int numVertices = 0;
	int numTris = 0;
	int numIndices = 0;

	std::vector<Vertex> vertexBufferData;
	std::vector<uint16_t> indexBufferData; //setting this to uint16_t fixed the issue. INVESTIGATE THIS.

	~Mesh() = default;
};

struct Object 
{
	int numVertices = 0;
	int textureIndex = -1;
	glm::vec3 mCenter = glm::vec3(0.f);
	Buffer vertexBuffer;
	Buffer indexBuffer;
	VkPipelineLayout* mPipelineLayout = nullptr;
	std::vector<Vertex> vertexBufferData;
	std::vector<uint16_t> indexBufferData;
	glm::mat4 mModelTransform;

	Object(const char* fileName, const char* textureName = nullptr, VkPipelineLayout* pipelineLayout = nullptr);
	Object() : mCenter(0.f), mModelTransform(1.f), vertexBuffer(), indexBuffer() {};
	void Draw(VkCommandBuffer cmdBuffer);
	void DestroyResources();
	~Object() = default;
};

void LoadMeshOBJ(const std::string& path, Object& obj);


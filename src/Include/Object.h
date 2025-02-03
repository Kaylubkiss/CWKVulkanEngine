#pragma once
#include "Common.h"
#include "Debug.h"
#include "Physics.h"
#include <iostream>
#include <string>


enum ColliderType
{
	NONE = 0,
	CUBE,
};


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
	DebugDrawObject debugDrawObject;
	PhysicsComponent mPhysicsComponent;
	VkPipelineLayout* mPipelineLayout = nullptr;
	std::vector<Vertex> vertexBufferData;
	std::vector<uint16_t> indexBufferData;
	
	glm::mat4 mModelTransform = glm::mat4(1.f);
	glm::vec3 mMaxLocalPoints = glm::vec3(0.f);
	glm::vec3 mMinLocalPoints = glm::vec3(0.f);

	Object(const char* fileName, bool willDebugDraw = false, const glm::mat4& modelTransform = glm::mat4(1.f), const char* textureName = nullptr, VkPipelineLayout* pipelineLayout = nullptr);

	void UpdateTexture(const char* textureName = nullptr);
	void UpdatePipelineLayout(VkPipelineLayout* pipelineLayout = nullptr);

	Object() : mCenter(0.f), mModelTransform(1.f), vertexBuffer(), indexBuffer(), mPhysicsComponent() {};
	void Update(const float& interpFactor);
	void Draw(VkCommandBuffer cmdBuffer);
	void DestroyResources();
	void InitPhysics(ColliderType cType, BodyType bType = BodyType::DYNAMIC);
	void willDebugDraw(bool option);
	void SetLinesArrayOffset(uint32_t index);
	void ComputeVertexNormals();
	/*void operator=(const Object& rhs) = default;*/
	~Object() 
	{
		Object::DestroyResources();
	}
};

void LoadMeshOBJ(const std::string& path, Object& obj);


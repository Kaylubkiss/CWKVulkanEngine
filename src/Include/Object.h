#pragma once
//#include "Debug.h"
#include "Physics.h"
#include "vkBuffer.h"
#include <iostream>
#include <string>


enum ColliderType
{
	NONE = 0,
	CUBE,
};

class Object 
{
	private:
		PhysicsComponent mPhysicsComponent;
		
		int numVertices = 0;
		int textureIndex = -1;
	
		vk::Buffer vertexBuffer;
		vk::Buffer indexBuffer;
		
		VkPipelineLayout* mPipelineLayout = nullptr;
	
		std::vector<Vertex> vertexBufferData;
		std::vector<uint16_t> indexBufferData;
		
		glm::vec3 mCenter = glm::vec3(0.f);
	
		glm::mat4 mModelTransform = glm::mat4(1.f);
		
		glm::vec3 mMaxLocalPoints = glm::vec3(0.f);
		glm::vec3 mMinLocalPoints = glm::vec3(0.f);
	
	public:
		Object(const VkPhysicalDevice p_device, const VkDevice l_device, const char* fileName, bool willDebugDraw = false, const glm::mat4& modelTransform = glm::mat4  (1.f), const char* textureName = nullptr, VkPipelineLayout* pipelineLayout = nullptr);
	
		void UpdateTexture(const char* textureName = nullptr);
		void UpdatePipelineLayout(const VkPipelineLayout pipelineLayout = nullptr);
	
		Object() : mCenter(0.f), mModelTransform(1.f), vertexBuffer(), indexBuffer(), mPhysicsComponent() {};
		~Object() = default;
		void Destroy(const VkDevice l_device);

		void Update(const float& interpFactor);
		void Draw(VkCommandBuffer cmdBuffer);
		void InitPhysics(ColliderType cType, BodyType bType = BodyType::DYNAMIC);
		/*void SetLinesArrayOffset(uint32_t index);*/
		void ComputeVertexNormals();
};

void LoadMeshOBJ(const std::string& path, Object& obj);


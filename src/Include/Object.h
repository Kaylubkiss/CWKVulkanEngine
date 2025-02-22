#pragma once
//#include "Debug.h"
#include "Physics.h"
#include "vkBuffer.h"
#include <iostream>
#include <string>
#include "vkMesh.h"



#define OBJECT_PATH "External/objects/"

class Object 
{
	private:
		Mesh mMesh;
		
		PhysicsComponent mPhysicsComponent;

		int textureIndex = -1;

		VkDescriptorSet mTextureDescriptor = VK_NULL_HANDLE;
		VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;
	
	public:
		Object(const VkPhysicalDevice p_device, const VkDevice l_device,
			const char* fileName, bool willDebugDraw,
			const glm::mat4& modelTransform);
	
		void UpdateTexture(const VkDescriptorSet textureDescriptor);
		void UpdatePipelineLayout(const VkPipelineLayout pipelineLayout = nullptr);
		void UpdatePhysicsComponent(PhysicsComponent* physComp);

		Object() = default;
		~Object() = default;
		void Destroy(const VkDevice l_device);

		void Update(const float& interpFactor);
		void Draw(VkCommandBuffer cmdBuffer);
		void InitPhysics(PhysicsSystem& appPhysics);
		/*void SetLinesArrayOffset(uint32_t index);*/
		void ComputeVertexNormals();

		friend void LoadMeshOBJ(const std::string& path, Object& obj);
};



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

		VkDescriptorSet textureDescriptorSet;

		bool debugDraw = false;
	
	public:
		Object(const VkPhysicalDevice p_device, const VkDevice l_device,
			const char* fileName, bool willDebugDraw,
			const glm::mat4& modelTransform);
	
		void UpdatePhysicsComponent(PhysicsComponent* physComp);
		void SetDebugDraw(bool option);

		Object() = default;
		~Object() = default;
		void Destroy(const VkDevice l_device);

		void Update(const float& interpFactor);
		void Draw(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout = VK_NULL_HANDLE);
		void InitPhysics(PhysicsSystem& appPhysics);
		/*void SetLinesArrayOffset(uint32_t index);*/
		void ComputeVertexNormals();

		void AddTextureDescriptor(VkDescriptorSet tDescriptorSet);

		friend void LoadMeshOBJ(const std::string& path, Object& obj);
};



#pragma once
//#include "Debug.h"
#include "Physics.h"
#include "vkBuffer.h"
#include <iostream>
#include <string>
#include "vkMesh.h"
#include "vkTexture.h"



#define OBJECT_PATH "External/objects/"

class Object 
{
	private:
		Mesh mMesh;
		
		glm::mat4  modelTransform = glm::mat4(1.f);

		PhysicsComponent mPhysicsComponent;

		VkDescriptorSet textureDescriptorSet = VK_NULL_HANDLE;

		bool debugDraw = false;
	
	public:
		Object(const VkPhysicalDevice p_device, const VkDevice l_device,
			const char* fileName, bool willDebugDraw = false);
	
		void UpdatePhysicsComponent(const PhysicsComponent* physComp);
		void UpdateModelTransform(const glm::mat4* modelTransform);
		void UpdateMesh(const Mesh* mesh);
		void SetDebugDraw(bool option);

		Object() = default;
		~Object() = default;
		void Destroy(const VkDevice l_device);

		void Update(const float& interpFactor);
		void Draw(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout = VK_NULL_HANDLE);
		void InitPhysics(PhysicsSystem& appPhysics);
		/*void SetLinesArrayOffset(uint32_t index);*/

		void AddTextureDescriptorSet(VkDescriptorSet textureDscSet);
};



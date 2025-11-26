#pragma once

#include "vkMesh.h"

#define OBJECT_PATH "External/objects/"

class Object 
{
	private:
		vk::Device* devicePtr = nullptr;

		Mesh mMesh;
		
		glm::mat4 modelTransform = glm::mat4(1.f);

		PhysicsComponent mPhysicsComponent;

		std::shared_ptr<VkDescriptorSet> textureDescriptorSet = nullptr;
	public:
		Object(const VkPhysicalDevice p_device, const VkDevice l_device,
			const char* fileName, bool willDebugDraw = false);
		
		Object(vk::Device* device);
		void UpdatePhysicsComponent(const PhysicsComponent* physComp);
		void UpdateModelTransform(const glm::mat4* modelTransform);
		void UpdateMesh(const Mesh* mesh);
		void UpdateDescriptorSet(std::vector<VkWriteDescriptorSet>& writeDescriptors, std::shared_ptr<VkDescriptorSet> descriptorSet);

		Object() = default;
		~Object() = default;
		void Destroy(const VkDevice l_device);

		void Update(const float& interpFactor);
		void Draw(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout = VK_NULL_HANDLE);
		void InitPhysics(PhysicsSystem& appPhysics);

};



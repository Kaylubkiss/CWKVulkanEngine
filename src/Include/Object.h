#pragma once

#include "IModel.h"

#define OBJECT_PATH "External/objects/"

struct ObjectCreateInfo
{
	//must fill out objName, even if there is no extension.
	const char* objName = "";
	const char* textureFileName = "";
	PhysicsComponent physicsComponent;
	bool hasPhysicsComponent = false;
	glm::mat4 modelTransform;
	vk::Device* devicePtr = nullptr;
};

class Object
{	
public:
	//Constructors
	Object() = default;
	Object( const ObjectCreateInfo& objectCI );
	//Destructors
	~Object() = default;	
	//Accessors
	uint32_t TextureIndex();
	//Mutators
	void UpdateTextureDescriptorOffset(uint32_t offset);
	void Update(const float& interpFactor);
	void InitPhysics();
	void Draw(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout = VK_NULL_HANDLE);
private:
	std::unique_ptr<IModel> m_model;
	PhysicsComponent m_physicsComponent;
	uint32_t m_textureIndex = 0;
};



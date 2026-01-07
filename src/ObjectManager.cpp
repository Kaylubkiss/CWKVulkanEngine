#include "ObjectManager.h"
#include "ThreadPool.h"
#include "vkInit.h"
#include "vkUtility.h"

namespace vk
{

	void ObjectManager::LoadObject(const ObjectCreateInfo& objectCI)
	{
		Object* newObject = new Object(objectCI.devicePtr);
		//just get the texture now to avoid asynchronous issues.
		this->textureSys->BindTextureToObject(objectCI.textureFileName, *newObject);

		ObjectCreateInfo deepyCopyCI = objectCI;
		deepyCopyCI.pModelTransform = objectCI.pModelTransform ? new glm::mat4(*objectCI.pModelTransform) : nullptr;
		deepyCopyCI.pPhysicsComponent = objectCI.pPhysicsComponent ? new PhysicsComponent(*objectCI.pPhysicsComponent) : nullptr;

		std::function<void()> parallelFunction = [this, deepyCopyCI, newObject]() {
			Mesh nMesh;
			nMesh.LoadOBJMesh((OBJECT_PATH + std::string(deepyCopyCI.objName)).c_str());

			newObject->UpdateMesh(&nMesh);
			newObject->UpdateModelTransform(deepyCopyCI.pModelTransform);
			newObject->UpdatePhysicsComponent(deepyCopyCI.pPhysicsComponent);

			objects[deepyCopyCI.objName].obj = newObject;

			delete deepyCopyCI.pModelTransform;
			delete deepyCopyCI.pPhysicsComponent;
		};

		mThreadWorkers.EnqueueTask(parallelFunction);
	}

	ObjectManager::ObjectManager() 
	{
		this->mThreadWorkers.Init(2);
	}

	void ObjectManager::Init(TextureManager* textureManager, VkPhysicalDevice physicalDevice, VkDevice device)
	{
		assert(physicalDevice != VK_NULL_HANDLE);
		assert(device != VK_NULL_HANDLE);
		assert(textureManager != nullptr);

		this->textureSys = textureManager;
		this->physicalDevice = physicalDevice;
		this->logicalDevice = device;
	}

	void ObjectManager::Destroy(const VkDevice l_device) 
	{
		this->mThreadWorkers.Terminate();

		if (l_device != VK_NULL_HANDLE) 
		{
			for (auto& obj : objects)
			{
				Object* curr_obj = obj.second.obj;

				if (curr_obj != nullptr)
				{
					curr_obj->Destroy(l_device);
				}
			}
		}
	}

	void ObjectManager::Update(float dt) 
	{
		for (auto& obj : objects)
		{
			Object* curr_obj = obj.second.obj;
			curr_obj->Update(dt);
		}
	}

	void ObjectManager::DrawObjects(VkCommandBuffer cmdBuffer, 
		VkPipelineLayout pipelineLayout, 
		VkDescriptorSet defaultDescriptorSet)
	{
		for (auto& obj : objects)
		{
			Object* curr_obj = obj.second.obj;
			
			if (curr_obj->HasTexture() == false && defaultDescriptorSet != VK_NULL_HANDLE)
			{
				vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &defaultDescriptorSet, 0, nullptr);
			}

			curr_obj->Draw(cmdBuffer, pipelineLayout);
		}
	}
}
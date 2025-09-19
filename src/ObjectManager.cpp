#include "ObjectManager.h"
#include "ThreadPool.h"
#include "vkInit.h"
#include "vkUtility.h"

namespace vk 
{

	void ObjectManager::LoadObjParallel(const ObjectCreateInfo& objInfo)
	{
		Object* newObject  = new Object(physicalDevice, logicalDevice, objInfo.objName, objInfo.debugWillDraw);
		newObject->UpdateModelTransform(objInfo.pModelTransform);
		newObject->UpdateMesh(objInfo.pMesh);
		newObject->UpdatePhysicsComponent(objInfo.pPhysicsComponent);

		delete objInfo.pModelTransform;
		delete objInfo.pMesh;
		delete objInfo.pPhysicsComponent;

		this->textureSys->BindTextureToObject(objInfo.textureFileName, *newObject);

		objects[objInfo.objName].obj = newObject;
	}

	void ObjectManager::LoadObject(const ObjectCreateInfo& objectCI)
	{

		ObjectCreateInfo deepCopy;
		deepCopy.pModelTransform = new glm::mat4(*objectCI.pModelTransform);
		deepCopy.pPhysicsComponent = objectCI.pPhysicsComponent ? new PhysicsComponent(*objectCI.pPhysicsComponent) : nullptr;
		deepCopy.pMesh = objectCI.pMesh ? new Mesh(*objectCI.pMesh) : nullptr;
		deepCopy.textureFileName = objectCI.textureFileName;
		deepCopy.objName = objectCI.objName;

		auto func = [this, deepCopy]() {
			ObjectManager::LoadObjParallel(deepCopy);
		};

		mThreadWorkers.EnqueueTask(func);
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

	void ObjectManager::Update(float dt) 
	{
		for (auto& obj : objects)
		{
			Object* curr_obj = obj.second.obj;
			curr_obj->Update(dt);
		}
	}


	void ObjectManager::DrawObjects(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout)
	{
		for (auto& obj : objects)
		{
			Object* curr_obj = obj.second.obj;
			curr_obj->Draw(cmdBuffer, pipelineLayout);
		}
	}
}
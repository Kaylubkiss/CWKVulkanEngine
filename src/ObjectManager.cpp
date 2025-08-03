#include "ObjectManager.h"
#include "ThreadPool.h"
#include "vkInit.h"
#include "vkUtility.h"


namespace vk 
{

	void ObjectManager::LoadObjParallel(const VkPhysicalDevice p_device, const VkDevice l_device, const char* name, const char* filename, bool willDebugDraw, const glm::mat4& modelTransform)
	{
		objects[name].obj  = new Object(p_device, l_device, filename, willDebugDraw, modelTransform);
	}

	void ObjectManager::LoadObject(const VkPhysicalDevice p_device, const VkDevice l_device, const char* filename, const glm::mat4& modelTransform, const char* texturename, const PhysicsComponent* physComp, bool willDebugDraw, const char* name)
	{
		PhysicsComponent* nPhysics = nullptr;

		if (physComp != nullptr) 
		{
			//make sure to delete this in the update function when given to the object!!
			nPhysics = new PhysicsComponent(*physComp);
		}

		if (name == nullptr) 
		{
			name = filename;
		}

		objectUpdateQueue.push_back({ nPhysics, texturename, name});

		std::function<void()> func = [this, p_device, l_device, modelTransform, name, filename, willDebugDraw] { 
			
			ObjectManager::LoadObjParallel(p_device, l_device, name, (std::string(filename)).c_str(), willDebugDraw, modelTransform); 	
		};

		mThreadWorkers.EnqueueTask(func);
	}

	ObjectManager::ObjectManager() 
	{
		this->mThreadWorkers.Init(2);
	}

	void ObjectManager::Init(TextureManager* textureManager) 
	{
		assert(textureManager != nullptr);
		this->textureSys = textureManager;
	}

	void ObjectManager::FinalizeObjects() 
	{
		//warning: can be very slow. Don't update object textures often at this point in development though.
		auto it = objectUpdateQueue.begin();

		while (it != objectUpdateQueue.end())
		{
			auto& objectInfo = objects[it->objName];

			if (objectInfo.obj != nullptr)
			{
				Object* curr_obj = objectInfo.obj;

				this->textureSys->BindTextureToObject(it->textureName, *curr_obj);

				if (it->physComp != nullptr)
				{
					curr_obj->UpdatePhysicsComponent(it->physComp);
					delete it->physComp;
				}
				
				objectInfo.isDoneLoading = true;

				it = objectUpdateQueue.erase(it);
			}
			else 
			{
				++it;
			}
		}

	}

	void ObjectManager::Update(float dt) 
	{

		if (!objectUpdateQueue.empty())
		{
			ObjectManager::FinalizeObjects();
		}

		for (auto& obj : objects)
		{
			auto& pair = obj.second;
			if (pair.isDoneLoading) 
			{
				Object* curr_obj = pair.obj;
				curr_obj->Update(dt);
			}
		}
	}


	void ObjectManager::DrawObjects(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout)
	{
		for (auto& obj : objects)
		{
			auto pair = obj.second;
			if (pair.isDoneLoading)
			{
				Object* curr_obj = pair.obj;
				curr_obj->Draw(cmdBuffer, pipelineLayout);
			}
		}
	}
}
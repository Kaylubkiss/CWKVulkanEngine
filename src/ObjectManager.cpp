#include "ObjectManager.h"
#include "ThreadPool.h"
#include "vkInit.h"
#include "vkUtility.h"


namespace vk 
{

	void ObjectManager::LoadObjParallel(const VkPhysicalDevice p_device, const VkDevice l_device, const char* name, const char* filename, bool willDebugDraw, const glm::mat4& modelTransform)
	{
		objects[name] = new Object(p_device, l_device, filename, willDebugDraw, modelTransform);
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

		/*std::function<void()> func = [this, p_device, l_device, modelTransform, name, filename, willDebugDraw] { ObjectManager::LoadObjParallel(p_device, l_device, name, (std::string(filename)).c_str(), willDebugDraw, modelTransform); };

		mThreadWorkers.EnqueueTask(func);*/

		objects[name] = new Object(p_device, l_device, filename, willDebugDraw, modelTransform);

	}

	void ObjectManager::Init()
	{
		this->mThreadWorkers.Init(1);
	}

	void ObjectManager::Update(TextureManager& textureManager, GraphicsSystem& graphicsSystem) 
	{
		//warning: can be very slow. Don't update object textures often at this point in development though.
		auto it = objectUpdateQueue.begin();

		while (it != objectUpdateQueue.end()) 
		{
			Object* curr_obj = objects[it->objName];

			if (curr_obj != nullptr)
			{
				textureManager.BindTextureToObject(it->textureName, graphicsSystem, *curr_obj);
				graphicsSystem.BindPipelineLayoutToObject(*curr_obj);
				
				if (it->physComp != nullptr) 
				{
					curr_obj->UpdatePhysicsComponent(it->physComp);
					delete it->physComp;
				}

				objectUpdateQueue.erase(it++);
			}
			else 
			{
				++it;
			}
		}
	}

	void ObjectManager::Update(float dt) {

		for (auto& obj : objects) 
		{
			Object* curr_obj = obj.second;

			if (curr_obj != nullptr)
			{
				curr_obj->Update(dt);
			}


		}
	}

	void ObjectManager::Draw(VkCommandBuffer cmdBuffer)
	{
		//this does not take advantage of the multithreading. Need some kind of array to offload from.
		for (auto& object : objects)
		{
			if (object.second != nullptr) 
			{
				object.second->Draw(cmdBuffer);
			}
		}
	}


	ObjectManager::ObjectManager()
	{
		//keep it to one thread.
	}
}
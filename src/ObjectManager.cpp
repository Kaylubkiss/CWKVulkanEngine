#include "ObjectManager.h"
#include "ThreadPool.h"
#include "vkInit.h"
#include "vkUtility.h"


namespace vk 
{

	void ObjectManager::LoadObjParallel(const VkPhysicalDevice p_device, const VkDevice l_device, const char* name, const char* filename, bool willDebugDraw, const glm::mat4& modelTransform)
	{
		objects[name].second  = new Object(p_device, l_device, filename, willDebugDraw, modelTransform);
		objects[name].first = true;

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

		std::function<void()> func = [this, p_device, l_device, modelTransform, name, filename, willDebugDraw] { ObjectManager::LoadObjParallel(p_device, l_device, name, (std::string(filename)).c_str(), willDebugDraw, modelTransform); };

		mThreadWorkers.EnqueueTask(func);

		//objects[name] = new Object(p_device, l_device, filename, willDebugDraw, modelTransform);

	}

	void ObjectManager::Init()
	{
		this->mThreadWorkers.Init(1);
	}

	void ObjectManager::AttachSystems(TextureManager* textureManager, GraphicsSystem* graphicsSystem) 
	{
		this->textureSys = textureManager;
		this->gfxSys = graphicsSystem;
	}

	void ObjectManager::FinalizeObjects() 
	{
		//warning: can be very slow. Don't update object textures often at this point in development though.
		auto it = objectUpdateQueue.begin();

		while (it != objectUpdateQueue.end())
		{
			std::pair<doneLoading, Object*>& pair = objects[it->objName];

			if (pair.first)
			{
				Object* curr_obj = pair.second;

				this->textureSys->BindTextureToObject(it->textureName, *this->gfxSys, *curr_obj);
				this->gfxSys->BindPipelineLayoutToObject(*curr_obj);

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

	void ObjectManager::Update(float dt, VkCommandBuffer cmdBuffer) 
	{
		if (!objectUpdateQueue.empty()) 
		{
			ObjectManager::FinalizeObjects();		
		}

		for (auto& obj : objects) 
		{
			auto pair = obj.second;
			if (pair.first) 
			{
				Object* curr_obj = pair.second;
				curr_obj->Update(dt);
				curr_obj->Draw(cmdBuffer);
			}
		}
	}


	ObjectManager::ObjectManager()
	{
		//keep it to one thread.
	}
}
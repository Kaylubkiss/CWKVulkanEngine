#include "ObjectManager.h"
#include "ThreadPool.h"
#include "vkInit.h"
#include "vkUtility.h"


namespace vk 
{

	void ObjectManager::LoadObjParallel(const VkPhysicalDevice p_device, const VkDevice l_device, const char* name, const char* filename, bool willDebugDraw, const glm::mat4& modelTransform)
	{
		objects[name].obj  = new Object(p_device, l_device, filename, willDebugDraw, modelTransform);
		objects[name].isDoneLoading = true;
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

	void ObjectManager::Init()
	{
		this->mThreadWorkers.Init(3);
	}

	void ObjectManager::AttachSystems(TextureManager* textureManager, ContextBase* graphicsSystem) 
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
			auto& objectInfo = objects[it->objName];

			if (objectInfo.isDoneLoading)
			{
				Object* curr_obj = objectInfo.obj;

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

	void ObjectManager::Update(float dt) 
	{

		if (!objectUpdateQueue.empty())
		{
			ObjectManager::FinalizeObjects();
		}
		else 
		{
			for (auto& obj : objects)
			{
				auto& pair = obj.second;
				Object* curr_obj = pair.obj;
				
				curr_obj->Update(dt);
			}
		}
	}


	ObjectManager::ObjectManager()
	{
		//keep it to one thread.
	}
}
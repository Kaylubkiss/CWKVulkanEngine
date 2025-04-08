#pragma once


#include "Object.h"
#include "vkGraphicsSystem.h"
#include <map>
#include <thread>
#include "Threadpool.h"
#include "TextureManager.h"


struct str_cmp 
{
	bool operator()(const char* a, char const* b) const 
	{
		return std::strcmp(a, b) < 0;
	}

};

namespace vk 
{

	typedef const char* ObjectName;
	typedef std::string TextureFileName;

	struct AsyncObjectInitInfo 
	{
		PhysicsComponent* physComp = nullptr;
		TextureFileName textureName = "";
		ObjectName objName = "";
	};

	class ObjectManager
	{
	public:
		ObjectManager();
		void Destroy(const VkDevice l_device) 
		{
			this->mThreadWorkers.Terminate();

			for (auto& obj : objects) 
			{
				Object* curr_obj = obj.second.second;

				if (curr_obj != nullptr)
				{
					curr_obj->Destroy(l_device);
				}
			}
		}
		~ObjectManager() = default;

		void LoadObject(const VkPhysicalDevice p_device, const VkDevice l_device, const char* filename = nullptr, const glm::mat4& modelTransform = glm::mat4(1.f), const char* texturename = nullptr, const PhysicsComponent* physComp = nullptr, bool willDebugDraw = false, const char* name = nullptr);

		Object* operator[](const char* name)
		{
			size_t count = objects.count(name);

			if (objects.count(name) > 0)
			{
				return objects[name].second;
			}
			else
			{
				return nullptr;
			}
		}

		void Init();

		void FinalizeObjects();
		void Update(float dt, VkCommandBuffer cmdBuffer);

		void AttachSystems(TextureManager* textureManager, GraphicsSystem* graphicsSystem);
	private:
		void LoadObjParallel(const VkPhysicalDevice p_device, const VkDevice l_device, const char* name = nullptr, const char* filename = nullptr, bool willDebugDraw = false, const glm::mat4& modelTransform = glm::mat4(1.f));

	//variables.
		ThreadPool mThreadWorkers;
		std::mutex map_mutex;

		typedef bool doneLoading;
		std::map<const char*, std::pair<doneLoading, Object*>, str_cmp> objects;
		std::list<AsyncObjectInitInfo> objectUpdateQueue;
		
		TextureManager* textureSys = nullptr;
		GraphicsSystem* gfxSys = nullptr;
	};
}

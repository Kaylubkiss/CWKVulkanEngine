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

	class ObjectManager
	{
	public:
		ObjectManager();
		void Destroy(const VkDevice l_device) 
		{
			for (auto obj : objects) 
			{
				if (obj.second != nullptr) 
				{
					obj.second->Destroy(l_device);
				}
			}
		}
		~ObjectManager() = default;

		void LoadObject(const VkPhysicalDevice p_device, const VkDevice l_device, const char* name = nullptr, const char* filename = nullptr, const char* texturename = nullptr, bool willDebugDraw = false, const glm::mat4& modelTransform = glm::mat4(1.f));

		Object* operator[](const char* name)
		{
			size_t count = objects.count(name);

			if (objects.count(name) > 0)
			{
				return objects[name];
			}
			else
			{
				return nullptr;
			}
		}

		void Draw(VkCommandBuffer cmdBuffer);
		void Init();

		void Update(TextureManager& textureManager, GraphicsSystem& graphicsSystem);

	private:
		void LoadObjParallel(const VkPhysicalDevice p_device, const VkDevice l_device, const char* name = nullptr, const char* filename = nullptr, bool willDebugDraw = false, const glm::mat4& modelTransform = glm::mat4(1.f));

		ThreadPool mThreadWorkers;
		std::map<const char*, Object*, str_cmp> objects;
		std::mutex map_mutex;

		typedef const char* ObjectName;
		typedef std::string TextureFileName;

		std::list<std::pair<ObjectName, TextureFileName>> objectUpdateQueue;

	public:


	};
}

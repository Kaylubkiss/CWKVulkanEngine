#pragma once

#include <map>
#include <thread>
#include "Object.h"
#include "Threadpool.h"
#include "TextureManager.h"

struct str_cmp 
{
	bool operator()(const char* a, char const* b) const 
	{
		return std::strcmp(a, b) < 0;
	}
};


struct ObjectCreateInfo 
{
	//must fill out objName, even if there is no extension.
	const char* objName = "";
	const char* textureFileName = "";
	Mesh* pMesh = nullptr;
	PhysicsComponent* pPhysicsComponent = nullptr;
	glm::mat4* pModelTransform = nullptr;
	bool debugWillDraw = false;
};

namespace vk 
{

	typedef const char* ObjectName;
	typedef std::string TextureFileName;

	class ObjectManager
	{
	public:

		ObjectManager();
		void Init(TextureManager* textureManager, VkPhysicalDevice physicalDevice, VkDevice device);
		~ObjectManager() = default;

		void Destroy(const VkDevice l_device) 
		{
			this->mThreadWorkers.Terminate();

			for (auto& obj : objects) 
			{
				Object* curr_obj = obj.second.obj;

				if (curr_obj != nullptr)
				{
					curr_obj->Destroy(l_device);
				}
			}
		}

		void LoadObject(const ObjectCreateInfo& objectCI);

		Object* operator[](const char* name)
		{
			size_t count = objects.count(name);

			if (objects.count(name) > 0)
			{
				return objects[name].obj;
			}
			else
			{
				return nullptr;
			}
		}


		size_t size()
		{
			return this->objects.size();
		}


		void DrawObjects(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout = VK_NULL_HANDLE);

		void Update(float dt);

	private:

	//variables.
		ThreadPool mThreadWorkers;
		std::mutex map_mutex;

		struct ObjectInfo 
		{
			bool isDoneLoading;
			Object* obj;
		};

		std::map<const char*, ObjectInfo, str_cmp> objects;
		
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkDevice logicalDevice = VK_NULL_HANDLE;

		TextureManager* textureSys = nullptr;
	};
}

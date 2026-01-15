#pragma once
#include "ThreadPool.h"

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
	vk::Device* devicePtr = nullptr;
	bool debugWillDraw = false;
};

namespace vk 
{

	typedef const char* ObjectName;
	typedef std::string TextureFileName;

	struct ObjectInfo
	{
		bool isDoneLoading;
		Object* obj;
	};

	class ObjectManager
	{
		public:

			ObjectManager();
			void Init(TextureManager* textureManager, VkPhysicalDevice physicalDevice, VkDevice device);
			~ObjectManager() = default;

			void Destroy(const VkDevice l_device);

			void LoadObject(const ObjectCreateInfo& objectCI);

			void DrawObjects(VkCommandBuffer cmdBuffer, 
				VkPipelineLayout pipelineLayout = VK_NULL_HANDLE);

			void Update(float dt);

			std::map<const char*, ObjectInfo, str_cmp>& Objects();

		private:

			ThreadPool mThreadWorkers;
			std::mutex map_mutex;

			std::map<const char*, ObjectInfo, str_cmp> objects;
			
			VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
			VkDevice logicalDevice = VK_NULL_HANDLE;

			TextureManager* textureSys = nullptr;
	};
}

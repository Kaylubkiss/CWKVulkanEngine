#include "ObjectManager.h"
#include "vkInit.h"
#include "vkUtility.h"

namespace vk
{

	void ObjectManager::LoadObject( const ObjectCreateInfo& objectCI )
	{
		std::function<void()> parallelFunction = [this, objectCI]()
		{
			m_objects[objectCI.objName] = std::make_unique<Object>(objectCI);
			m_textureManager->BindTextureToObject(objectCI.textureFileName, *m_objects[objectCI.objName].get());
		};

		m_threadWorkers.EnqueueTask(parallelFunction);
	}

	ObjectManager::ObjectManager( GraphicsContextInfo& contextInfo )
	{
		assert(contextInfo.devicePtr != nullptr);

		m_threadWorkers.Init(2);

		m_textureManager = std::make_unique<TextureManager>(contextInfo);

		c_devicePtr = contextInfo.devicePtr;
	}
	
	void ObjectManager::SyncIO() 
	{
		m_textureManager->FinishTextureLayoutTransition();
	}
	
	void ObjectManager::Update(float dt) 
	{
		for (auto& obj : m_objects)
		{
			Object* curr_obj = obj.second.get();
			curr_obj->Update(dt);
		}
	}

	std::map<const char*, std::unique_ptr<Object>, str_cmp>& ObjectManager::Objects()
	{
		return m_objects;
	}

	void ObjectManager::DrawObjects( VkCommandBuffer cmdBuffer, 
		VkPipelineLayout pipelineLayout )
	{
		for (auto& obj : m_objects)
		{
			Object* curr_obj = obj.second.get();
			curr_obj->Draw(cmdBuffer, pipelineLayout);
		}
	}
}
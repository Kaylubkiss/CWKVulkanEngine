#include "ObjectManager.h"


void ObjectManager::LoadObject( const ObjectCreateInfo& objectCI )
{
	std::function<void()> parallelFunction = [this, objectCI]()
	{
		//note: m_textureManager is also internally thread safe.
		auto newObject = std::make_unique<Object>(objectCI, m_textureManager);

		{
			std::unique_lock<std::mutex> lock(m_objectMutex);
			if (m_objects.contains(objectCI.objName) == false)
			{
				m_objects[objectCI.objName] = std::move(newObject);
			}
		}
	};

	m_threadWorkers.EnqueueTask(parallelFunction);
}

void ObjectManager::Destroy()
{
	m_threadWorkers.Terminate();

	m_textureManager.Destroy();

	m_objects.clear(); //destroy objects with ~Object();
}

ObjectManager::ObjectManager( const std::weak_ptr<vk::GraphicsContextInfo>& contextInfo )
{
	assert(contextInfo.expired() == false);

	assert(contextInfo.lock()->devicePtr != nullptr);

	auto sharedContextInfo = contextInfo.lock();

	m_threadWorkers.Init(2);

	m_textureManager.Init(sharedContextInfo);

	c_devicePtr = sharedContextInfo->devicePtr;
}

void ObjectManager::Update( float dt ) const
{
	for (auto& obj : m_objects)
	{
		Object* curr_obj = obj.second.get();
		curr_obj->Update(dt);
	}
}

//NOTE: ONLY CALL THIS ON THE MAIN THREAD!!
bool ObjectManager::SyncIO( uint32_t currentFrame, VkSemaphore textureUploadSemaphore )
{
	return m_textureManager.UploadTextureDataToGPU(currentFrame, textureUploadSemaphore);
}

void ObjectManager::DrawObjects( const vk::DrawInfo& drawInfo ) const
{
	for (auto& obj : m_objects)
	{
		Object* curr_obj = obj.second.get();
		curr_obj->Draw(drawInfo);
	}
}
#include "AssetManager.h"


void AssetManager::LoadObject( const ObjectCreateInfo& objectCI )
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

void AssetManager::Destroy()
{
	m_threadWorkers.Terminate();

	m_textureManager.Destroy();

	m_objects.clear(); //destroy objects with ~Object();
}

AssetManager::AssetManager( const std::weak_ptr<vk::GraphicsContextInfo>& contextInfo )
{
	assert(contextInfo.expired() == false);

	assert(contextInfo.lock()->devicePtr != nullptr);

	auto sharedContextInfo = contextInfo.lock();

	m_threadWorkers.Init(2);

	m_textureManager.Init(sharedContextInfo);

	c_devicePtr = sharedContextInfo->devicePtr;
}

void AssetManager::Update( float dt ) const
{
	for (auto& obj : m_objects)
	{
		Object* curr_obj = obj.second.get();
		curr_obj->Update(dt);
	}
}


const TextureManager& AssetManager::GetTextureManager() const
{
	return m_textureManager;
}

//NOTE: ONLY CALL THIS ON THE MAIN THREAD!!
bool AssetManager::SyncIO( uint32_t currentFrame, VkSemaphore textureUploadSemaphore )
{
	return m_textureManager.UploadTextureDataToGPU(currentFrame, textureUploadSemaphore);
}

void AssetManager::DrawObjects( const vk::DrawInfo& drawInfo ) const
{
	for (auto& obj : m_objects)
	{
		Object* curr_obj = obj.second.get();
		curr_obj->Draw(drawInfo);
	}
}
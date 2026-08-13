#include "AssetLoader.h"
#include <ranges>

void AssetLoader::LoadObject( ObjectCreateInfo& objectCI )
{
	objectCI.textureManagerPtr = m_textureManagerPtr;
	objectCI.devicePtr = c_devicePtr;

	std::function parallelFunction = [this, objectCI]()
	{
		{
			std::shared_lock lock(m_objectMutex);
			if (m_objects.contains(objectCI.objName) == true)
			{
				return;
			}
		}

		//note: m_textureManager is also internally thread safe.
		auto newObject = std::make_unique<Object>(objectCI, *objectCI.textureManagerPtr);
		{
			std::unique_lock lock(m_objectMutex);
			if (m_objects.contains(objectCI.objName) == false)
			{
				if (objectCI.physicsInfo.has_value())
				{
					newObject->InitPhysics(objectCI.physicsInfo.value());
				}

				m_objects[objectCI.objName] = std::move(newObject);
			}
		}
	};

	m_threadWorkers.EnqueueTask(parallelFunction);
}

void AssetLoader::Destroy()
{
	m_threadWorkers.Terminate();
	std::unique_lock lock(m_objectMutex);
	m_objects.clear();
}

std::shared_ptr<Object> AssetLoader::GetObject( const std::string& objectName )
{
	std::shared_lock lock(m_objectMutex);
	auto it = m_objects.find( objectName );
	if (it != m_objects.end())
	{
		return it->second;
	}

	return {};
}

void AssetLoader::Init( const vk::Device* devicePtr, vk::TextureManager* textureManagerPtr, size_t workerThreadCount )
{
	assert(devicePtr != nullptr);
	assert(textureManagerPtr != nullptr);
	assert(workerThreadCount > 0);

	m_threadWorkers.Init(workerThreadCount);
	c_devicePtr = devicePtr;
	m_textureManagerPtr = textureManagerPtr;

}

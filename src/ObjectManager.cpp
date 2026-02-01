#include "ObjectManager.h"


void ObjectManager::LoadObject( const ObjectCreateInfo& objectCI )
{
	std::function<void()> parallelFunction = [this, objectCI]()
	{
		//note: m_textureManager is also internally thread safe.
		auto newObject = std::make_unique<Object>(objectCI, *m_textureManager.get());

		{
			std::unique_lock<std::mutex> lock(m_objectMutex);
			m_objects[objectCI.objName] = std::move(newObject);
		}
	};

	m_threadWorkers.EnqueueTask(parallelFunction);
}

ObjectManager::ObjectManager( vk::GraphicsContextInfo& contextInfo )
{
	assert(contextInfo.devicePtr != nullptr);

	m_threadWorkers.Init(1);

	m_textureManager = std::make_unique<TextureManager>(contextInfo);

	c_devicePtr = contextInfo.devicePtr;
}


void ObjectManager::Update(float dt) const
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

std::unique_ptr<TextureManager>& ObjectManager::GetTextureManager()
{
	return m_textureManager;
}

void ObjectManager::DrawObjects( const vk::DrawInfo& drawInfo ) const
{
	for (auto& obj : m_objects)
	{
		Object* curr_obj = obj.second.get();
		curr_obj->Draw(drawInfo);
	}
}
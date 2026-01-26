#include "ObjectManager.h"


void ObjectManager::LoadObject( const ObjectCreateInfo& objectCI )
{
	std::function<void()> parallelFunction = [this, objectCI]()
	{
		m_objects[objectCI.objName] = std::make_unique<Object>(objectCI, *m_textureManager.get());
	};

	m_threadWorkers.EnqueueTask(parallelFunction);
}

ObjectManager::ObjectManager( vk::GraphicsContextInfo& contextInfo )
{
	assert(contextInfo.devicePtr != nullptr);

	m_threadWorkers.Init(2);

	m_textureManager = std::make_unique<TextureManager>(contextInfo);

	c_devicePtr = contextInfo.devicePtr;
}

void ObjectManager::SyncIO() const
{
	m_textureManager->FinishTextureLayoutTransition();
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

void ObjectManager::DrawObjects( const vk::DrawInfo& drawInfo ) const
{
	for (auto& obj : m_objects)
	{
		Object* curr_obj = obj.second.get();
		curr_obj->Draw(drawInfo);
	}
}
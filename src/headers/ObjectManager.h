#pragma once
#include "Object.h"
#include "ThreadPool.h"
class TextureManager;

class ObjectManager
{
public:
	ObjectManager() = default;
	ObjectManager( const std::weak_ptr<vk::GraphicsContextInfo>& contextInfo );
	~ObjectManager() = default;
	void Destroy();

	//Modifiers
	void LoadObject( const ObjectCreateInfo& objectCI );
	void DrawObjects( const vk::DrawInfo& drawInfo ) const;
	void Update( float dt ) const;

	//returns whether or not a command was recorded.
	bool SyncIO( uint32_t currentFrame, VkSemaphore textureUploadSemaphore );
private:
	std::mutex m_objectMutex;
	vk::Device* c_devicePtr = nullptr;
	std::unordered_map<const char*, std::unique_ptr<Object>> m_objects;
	TextureManager m_textureManager;
	ThreadPool m_threadWorkers; //this needs to be destroyed first.

	//TODO: make buffer pool for geometry.
};


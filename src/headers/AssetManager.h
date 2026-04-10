#pragma once
#include "Object.h"
#include "ThreadPool.h"
#include <shared_mutex>

class TextureManager;

typedef std::unordered_map<std::string, std::unique_ptr<Object>> ObjectMap;

class AssetManager
{
public:
	AssetManager() = default;
	~AssetManager() = default;

	//Modifiers
	void Init( vk::Device* devicePtr, TextureManager* textureManagerPtr, size_t workerThreadCount );
	void Destroy();
	void LoadObject( const ObjectCreateInfo& objectCI );
	void DrawObjects( const vk::DrawInfo& drawInfo ) const;
	void Update( float dt );
	//returns whether or not a command was recorded.
	bool SyncIO( uint32_t currentFrame, VkSemaphore textureUploadSemaphore );
protected:
	void InitTestScene();
private:
	mutable std::shared_mutex m_objectMutex; //"mutable" to bypass const methods
	vk::Device* c_devicePtr = nullptr;
	ObjectMap m_objects;
	ThreadPool m_threadWorkers; //this needs to be destroyed first.
	TextureManager* m_textureManagerPtr = nullptr;

	//TODO: make buffer pool for geometry.
};


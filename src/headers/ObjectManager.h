#pragma once
#include "Object.h"
#include "ThreadPool.h"
#include "TextureManager.h"

struct str_cmp
{
	bool operator()(const char* a, char const* b) const
	{
		return std::strcmp(a, b) < 0;
	}
};

struct TextureSyncPrimitives
{
	VkCommandBuffer cmdBuffer;

};

typedef const char* ObjectName;
typedef std::string TextureFileName;

class ObjectManager
{
public:
	ObjectManager() = default;
	ObjectManager( std::shared_ptr<vk::GraphicsContextInfo>& contextInfo );
	~ObjectManager() = default;
	void Destroy();

	//Accessors
	std::map<const char*, std::unique_ptr<Object>, str_cmp>& Objects();

	//Modifiers
	void LoadObject( const ObjectCreateInfo& objectCI );
	void DrawObjects( const vk::DrawInfo& drawInfo ) const;
	void Update( float dt ) const;

	//returns whether or not a command was recorded.
	bool SyncIO( uint32_t currentFrame, VkSemaphore textureUploadSemaphore );
private:
	std::mutex m_objectMutex;
	vk::Device* c_devicePtr = nullptr;
	std::map<const char*, std::unique_ptr<Object>, str_cmp> m_objects;
	TextureManager m_textureManager;
	ThreadPool m_threadWorkers; //this needs to be destroyed first.
};


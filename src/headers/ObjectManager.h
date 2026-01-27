#pragma once
#include "ThreadPool.h"
#include "Object.h"
#include "TextureManager.h"

struct str_cmp
{
	bool operator()(const char* a, char const* b) const
	{
		return std::strcmp(a, b) < 0;
	}
};


typedef const char* ObjectName;
typedef std::string TextureFileName;

class ObjectManager
{
public:

	ObjectManager() = default;
	ObjectManager( vk::GraphicsContextInfo& contextInfo );
	~ObjectManager() = default;

	//Accessors
	std::map<const char*, std::unique_ptr<Object>, str_cmp>& Objects();

	//Modifiers
	void SyncIO() const;
	void LoadObject( const ObjectCreateInfo& objectCI );
	void DrawObjects( const vk::DrawInfo& drawInfo ) const;
	void Update( float dt ) const;

private:
	std::mutex m_objectMutex;
	ThreadPool m_threadWorkers;

	vk::Device* c_devicePtr = nullptr;

	std::map<const char*, std::unique_ptr<Object>, str_cmp> m_objects;
	std::unique_ptr<TextureManager> m_textureManager;
};


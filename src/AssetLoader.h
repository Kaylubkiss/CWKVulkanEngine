#ifndef APPLICATION_MANAGER_HPP
#define APPLICATION_MANAGER_HPP

#include "Object.h"
#include "ThreadPool.h"
#include <shared_mutex>
#include "SceneDefinitions.h"
#include "vkDevice.h"

// (7.20.26) Asset manager is doing too much.
// Asset Manager:
//	- load requested assets
//	- deallocate assets if scene no longer uses them/program closes.

//	we are currently loading and updating the resources themselves. (2 responsibilities)

class vk::TextureManager;

class AssetLoader
{
public:
	AssetLoader() = default;
	~AssetLoader() = default;

	[[nodiscard]] std::shared_ptr<Object> GetObject( const std::string& objectName );

	void Init( const vk::Device* devicePtr, vk::TextureManager* textureManagerPtr, size_t workerThreadCount );
	void Destroy();
	void LoadObject( ObjectCreateInfo& objectCI );
private:
	mutable std::shared_mutex m_objectMutex; //"mutable" to bypass const methods
	ObjectMap m_objects{};
	ObjectMap m_transparentObjects{};
	ThreadPool m_threadWorkers; //this needs to be destroyed first.
	vk::TextureManager* m_textureManagerPtr = nullptr;
	const vk::Device* c_devicePtr = nullptr;
};

#endif


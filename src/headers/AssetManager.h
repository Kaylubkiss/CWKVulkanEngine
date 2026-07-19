#ifndef APPLICATION_MANAGER_HPP
#define APPLICATION_MANAGER_HPP

#include "Object.h"
#include "ThreadPool.h"
#include <shared_mutex>
#include <unordered_set>

namespace vk
{
	class TextureManager;
}

typedef std::unordered_map<std::string, std::shared_ptr<Object>> ObjectMap;

struct SceneView
{
	std::vector<std::weak_ptr<Object>> opaqueObjects;
	std::vector<std::weak_ptr<Object>> transparentObjects;

	void Reset()
	{
		opaqueObjects.clear();
		transparentObjects.clear();
	}
};

class AssetManager
{
public:
	AssetManager() = default;
	~AssetManager() = default;

	[[nodiscard]] SceneView GetSceneView() const;

	void Init( const vk::Device* devicePtr, vk::TextureManager* textureManagerPtr, size_t workerThreadCount );
	void Destroy();
	void LoadObject( const ObjectCreateInfo& objectCI );
	void DrawObjects( const vk::DrawInfo& drawInfo ) const;
	void Update( float dt );

protected:
	void InitTestScene();
private:
	mutable std::shared_mutex m_objectMutex; //"mutable" to bypass const methods
	const vk::Device* c_devicePtr = nullptr;
	ObjectMap m_objects;
	ObjectMap m_transparentObjects;
	ThreadPool m_threadWorkers; //this needs to be destroyed first.
	vk::TextureManager* m_textureManagerPtr = nullptr;
	SceneView m_sceneView;

	//TODO: make buffer pool for geometry.
};

#endif


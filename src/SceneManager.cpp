#include "SceneManager.h"
#include "Input.h"
#include <ranges>
#include "Camera.h"
#include "ObjectParser.h"

void SceneManager::Init( AssetLoader& assetLoader )
{
	c_assetLoader = &assetLoader;

	glm::vec3 eye =  { 0.f, 0.f, 10.f };
	glm::vec3 lookDirection = { 0.f, 0.f, -1.f };
	glm::vec3 up = { 0.f, 1.f, 0.f };

	m_scene.m_camera = std::make_shared<Camera>(eye, lookDirection, up);

	InitTestScene();
}

[[nodiscard]] SceneView SceneManager::GetSceneView() const
{
	SceneView newSceneView;

	newSceneView.opaqueObjects = m_scene.m_objects;
	newSceneView.camera = m_scene.m_camera;

	return newSceneView;
}

void SceneManager::GrabRequestedObjects()
{
	if (!m_requestedObjects.empty())
	{
		auto it = m_requestedObjects.begin();
		while (it != m_requestedObjects.end())
		{
			auto object = c_assetLoader->GetObject( *it );
			if (object != nullptr)
			{
				m_scene.m_objects.emplace_back(object);
				it = m_requestedObjects.erase(it);
			}
			else
			{
				++it;
			}
		}
	}
}

void SceneManager::Update( float physicsInterp, float dt )
{
    GrabRequestedObjects();

    for (auto& obj : m_scene.m_objects)
    {
        if (obj) //null check per object?
        {
            obj->Update(physicsInterp);
        }
    }

	MoveCamera(*m_scene.m_camera, dt );
}



void SceneManager::InitTestScene()
{
	std::vector<ObjectCreateInfo> object_list;

	_TEST_ReadObjects("art/json_scenes/test_object_list.json", object_list);

	for (auto& obj : object_list)
	{
		c_assetLoader->LoadObject(obj);

		m_requestedObjects.push_back(obj.objName);
	}
}
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
    ObjectCreateInfo objectCI = {};
	PhysicsInitInfo physInfo;

	_TEST_ReadObject("art/json_scenes/test_object.json", objectCI);

	c_assetLoader->LoadObject(objectCI);

	m_requestedObjects.push_back(objectCI.objName);

	//object 2 - cube
	objectCI = {};

	glm::vec3 cubePosition   = { 1.0, 20, -5.f };

	physInfo.bodyType = reactphysics3d::BodyType::DYNAMIC;
	physInfo.colliderType = PhysicsInitInfo::ColliderType::CUBE;

	objectCI.objName = "cube.obj";
	//NOTE: cube.obj doesn't have UVs.
	objectCI.textureFileNames = { "art/extern-textures/myface.JPG" } ;
	objectCI.physicsInfo = physInfo;
	objectCI.modelTransform = glm::translate(glm::mat4(1.f), glm::vec3(cubePosition));

	c_assetLoader->LoadObject(objectCI);

	m_requestedObjects.push_back(objectCI.objName);

	//object 3 - base
	objectCI = {};

	physInfo = {};
	physInfo.bodyType = reactphysics3d::BodyType::STATIC;
	physInfo.colliderType = PhysicsInitInfo::ColliderType::CUBE;

	objectCI.objName = "base.obj";
	objectCI.textureFileNames = { "art/extern-textures/wood-floor.png" } ;
	objectCI.physicsInfo = physInfo;
	objectCI.modelTransform = glm::translate(glm::mat4(1.f), glm::vec3(0, -5.f, 0)) *
		glm::scale(glm::mat4(1.f), glm::vec3(30.f));

	c_assetLoader->LoadObject(objectCI);

	m_requestedObjects.push_back(objectCI.objName);

	/*objectCI = {};

	objectCI.objName = "AnimatedCube/glTF/AnimatedCube.gltf";
	objectCI.modelTransform = glm::translate(glm::mat4(1.f), glm::vec3(-3.5, -1.5f, 0));

	assetManagerPtr->LoadObject(objectCI);

	m_requestedObjects.push_back(objectCI.objName);
	*/

	objectCI = {};

	objectCI.objName = "SciFiHelmet/glTF/SciFiHelmet.gltf";
	objectCI.modelTransform = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 1.f, 0));

	c_assetLoader->LoadObject(objectCI);

	m_requestedObjects.push_back(objectCI.objName);


	objectCI = {};

	objectCI.modelTransform = glm::translate(glm::mat4(1.f), glm::vec3(0.5f, -0.5f, 8)) *
		glm::scale(glm::mat4(1.f), glm::vec3(3));
	objectCI.objName = "DiffuseTransmissionTeacup/glTF/DiffuseTransmissionTeacup.gltf";

	c_assetLoader->LoadObject(objectCI);

	m_requestedObjects.push_back(objectCI.objName);
}
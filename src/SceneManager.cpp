#include "SceneManager.h"
#include <ranges>


void SceneManager::Init( AssetManager& assetManager )
{
	assetManagerPtr = &assetManager;

	InitTestScene();
}

[[nodiscard]] SceneView SceneManager::GetSceneView() const
{
	SceneView newSceneView;

	newSceneView.opaqueObjects.resize(m_objects.size());
	for (const auto& object : m_objects | std::views::values)
	{
		newSceneView.opaqueObjects.push_back(object);
	}

	return newSceneView;
}

void SceneManager::Update( float dt )
{
    GrabRequestedObjects();

    for (auto& val : m_objects | std::views::values)
    {
        auto obj = val.lock();

        if (obj) //null check per object?
        {
            obj->Update(dt);
        }
    }
}

void SceneManager::GrabRequestedObjects()
{
    if (!m_requestedObjects.empty())
    {
        auto it = m_requestedObjects.begin();
        while (it != m_requestedObjects.end())
        {
            auto object = assetManagerPtr->GetObject( *it );
            if (object != nullptr)
            {
                m_objects[*it] = object;
                it = m_requestedObjects.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
}

void SceneManager::InitTestScene()
{
    ObjectCreateInfo objectCI = {};

	glm::vec3 cubePosition   = { 1.0, 20, -5.f };
	glm::vec3 freddyPosition = { 1.5f, 1.0, 3.f };

	//object 1 - freddy
	objectCI.objName = "freddy.obj";
	objectCI.textureFileNames = { "art/extern-textures/myface.JPG" } ;
	objectCI.modelTransform = glm::translate(glm::mat4(1.f), freddyPosition) *
		glm::scale(glm::mat4(1.f), glm::vec3(3.f));

	assetManagerPtr->LoadObject(objectCI);

	m_requestedObjects.push_back(objectCI.objName);

	//object 2 - cube
	objectCI = {};

	PhysicsComponent physicsComponent;
	physicsComponent.bodyType = BodyType::DYNAMIC;
	physicsComponent.colliderType = PhysicsComponent::ColliderType::CUBE;

	objectCI.objName = "cube.obj";
	//NOTE: cube.obj doesn't have UVs.
	objectCI.textureFileNames = { "art/extern-textures/myface.JPG" } ;
	objectCI.physicsComponent = physicsComponent;
	objectCI.hasPhysicsComponent = true;
	objectCI.modelTransform = glm::translate(glm::mat4(1.f), glm::vec3(cubePosition));

	assetManagerPtr->LoadObject(objectCI);

	m_requestedObjects.push_back(objectCI.objName);

	//object 3 - base
	objectCI = {};

	physicsComponent.bodyType = reactphysics3d::BodyType::STATIC;

	objectCI.objName = "base.obj";
	objectCI.textureFileNames = { "art/extern-textures/wood-floor.png" } ;
	objectCI.physicsComponent = physicsComponent;
	objectCI.modelTransform = glm::translate(glm::mat4(1.f), glm::vec3(0, -5.f, 0)) *
		glm::scale(glm::mat4(1.f), glm::vec3(30.f));
	objectCI.hasPhysicsComponent = true;

	assetManagerPtr->LoadObject(objectCI);

	m_requestedObjects.push_back(objectCI.objName);

	objectCI = {};

	objectCI.objName = "AnimatedCube/glTF/AnimatedCube.gltf";
	objectCI.modelTransform = glm::translate(glm::mat4(1.f), glm::vec3(-3.5, -1.5f, 0));

	assetManagerPtr->LoadObject(objectCI);

	m_requestedObjects.push_back(objectCI.objName);

	objectCI = {};

	objectCI.objName = "SciFiHelmet/glTF/SciFiHelmet.gltf";
	objectCI.modelTransform = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 1.f, 0));

	assetManagerPtr->LoadObject(objectCI);

	m_requestedObjects.push_back(objectCI.objName);

	objectCI = {};

	objectCI.modelTransform = glm::translate(glm::mat4(1.f), glm::vec3(0.5f, -0.5f, 8)) *
		glm::scale(glm::mat4(1.f), glm::vec3(3));
	objectCI.objName = "DiffuseTransmissionTeacup/glTF/DiffuseTransmissionTeacup.gltf";

	assetManagerPtr->LoadObject(objectCI);

	m_requestedObjects.push_back(objectCI.objName);
}
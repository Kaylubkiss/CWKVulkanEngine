#include "SceneManager.h"

void SceneManager::Update(float dt)
{
	float physicsDt = m_physicsWorld.CalculateInterpFactor(dt);

	Scene& activeScene =  m_scenes[m_activeScene];

	for (auto& object : activeScene.m_objects)
	{
		object->Update(physicsDt);
	}
}

Scene& SceneManager::GetActiveScene()
{
	return m_scenes[m_activeScene];
}

void SceneManager::Init( ResourceManager* resourceManagerPtr )
{
	m_resourceManagerPtr = resourceManagerPtr;
}

void SceneManager::AddScene()
{
	ResourceManager& resourceManager = *m_resourceManagerPtr;

	m_scenes.resize(m_scenes.size() + 1);
	Scene& scene = m_scenes.back();

	ObjectCreateInfo objectCI = {};

	//object 1 - freddy
	objectCI = {};

	objectCI.objName = "freddy.obj";
	objectCI.textureFilename = "assets/textures/myface.JPG";
	objectCI.modelTransform = glm::translate(glm::mat4(1.f), { 1.5f, 1.0, 3.f }) *
		glm::scale(glm::mat4(1.f), glm::vec3(3.f));

	//auto textureHandle = resourceManager.Load<vk::Texture>(objectCI.textureFilename);
	auto textureHandle = (resourceManager.AsyncLoad<vk::Texture>(objectCI.textureFilename));

	//object 2 - cube
	objectCI = {};
	PhysicsComponent physicsComponent;
	physicsComponent.bodyType = BodyType::DYNAMIC;
	physicsComponent.colliderType = PhysicsComponent::ColliderType::CUBE;

	objectCI.objName = "cube.obj";
	//NOTE: cube.obj doesn't have UVs.
	objectCI.textureFilename = "assets/textures/myface.JPG";
	objectCI.physicsComponent = physicsComponent;
	objectCI.hasPhysicsComponent = true;
	objectCI.modelTransform = glm::translate(glm::mat4(1.f), { 1.0, 20, -5.f });

	auto texture2Handle = (resourceManager.AsyncLoad<vk::Texture>("assets/textures/wood-floor.png"));

	//object 3 - base
	/*objectCI= {};

	physicsComponent.bodyType = reactphysics3d::BodyType::STATIC;

	objectCI.objName = "base.obj";
	objectCI.textureFilename = "assets/textures/wood-floor.png";
	objectCI.physicsComponent = physicsComponent;
	objectCI.modelTransform = glm::translate(glm::mat4(1.f), glm::vec3(0, -5.f, 0)) *
		glm::scale(glm::mat4(1.f), glm::vec3(30.f));
	objectCI.hasPhysicsComponent = true;

	//object 4 - cube
	objectCI = {};
	objectCI.modelTransform = glm::translate(glm::mat4(1.f), glm::vec3(-3.5, -1.5f, 0));
	objectCI.objName = "AnimatedCube/glTF/AnimatedCube.gltf";

	//object 5 - helmet
	objectCI = {};
	objectCI.modelTransform = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 1.f, 0));
	objectCI.objName = "SciFiHelmet/glTF/SciFiHelmet.gltf";

	//object 6 - teacup
	objectCI = {};
	objectCI.modelTransform = glm::translate(glm::mat4(1.f), glm::vec3(0.5f, -0.5f, 8)) *
		glm::scale(glm::mat4(1.f), glm::vec3(3));
	objectCI.objName = "DiffuseTransmissionTeacup/glTF/DiffuseTransmissionTeacup.gltf";*/
	texture2Handle.wait();
	textureHandle.wait();
}
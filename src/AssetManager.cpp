#include "AssetManager.h"


void AssetManager::LoadObject( const ObjectCreateInfo& objectCI )
{
	std::function<void()> parallelFunction = [this, objectCI]()
	{
		{
			std::shared_lock lock(m_objectMutex);
			if (m_objects.contains(objectCI.objName) == true)
			{
				return;
			}
		}

		assert(objectCI.textureManagerPtr != nullptr);

		//note: m_textureManager is also internally thread safe.
		auto newObject = std::make_unique<Object>(objectCI, *objectCI.textureManagerPtr);
		{
			std::unique_lock lock(m_objectMutex);
			if (m_objects.contains(objectCI.objName) == false)
			{
				m_objects[objectCI.objName] = std::move(newObject);
			}
		}
	};

	m_threadWorkers.EnqueueTask(parallelFunction);
}

void AssetManager::Destroy()
{
	m_threadWorkers.Terminate();

	m_objects.clear(); //destroy objects with ~Object();
}

void AssetManager::Init( vk::Device* devicePtr, TextureManager* textureManagerPtr, size_t workerThreadCount )
{
	assert(devicePtr != nullptr);
	assert(textureManagerPtr != nullptr);
	assert(workerThreadCount > 0);

	m_threadWorkers.Init(workerThreadCount);

	c_devicePtr = devicePtr;

	m_textureManagerPtr = textureManagerPtr;

	InitTestScene();
}

void AssetManager::InitTestScene()
{
	ObjectCreateInfo objectCI = {};

	glm::vec3 cubePosition   = { 1.0, 20, -5.f };
	glm::vec3 freddyPosition = { 1.5f, 1.0, 3.f };

	//object 1 - freddy
	objectCI.objName = "freddy.obj";
	objectCI.textureFileName = "art/extern-textures/myface.JPG";
	objectCI.modelTransform = glm::translate(glm::mat4(1.f), freddyPosition) *
		glm::scale(glm::mat4(1.f), glm::vec3(3.f));
	objectCI.devicePtr = c_devicePtr;
	objectCI.textureManagerPtr = m_textureManagerPtr;

	LoadObject(objectCI);

	//object 2 - cube
	objectCI = {};

	PhysicsComponent physicsComponent;
	physicsComponent.bodyType = BodyType::DYNAMIC;
	physicsComponent.colliderType = PhysicsComponent::ColliderType::CUBE;

	objectCI.objName = "cube.obj";
	//NOTE: cube.obj doesn't have UVs.
	objectCI.textureFileName = "art/extern-textures/myface.JPG";
	objectCI.physicsComponent = physicsComponent;
	objectCI.hasPhysicsComponent = true;
	objectCI.modelTransform = glm::translate(glm::mat4(1.f), glm::vec3(cubePosition));
	objectCI.devicePtr = c_devicePtr;
	objectCI.textureManagerPtr = m_textureManagerPtr;

	LoadObject(objectCI);

	//object 3 - base
	objectCI = {};

	physicsComponent.bodyType = reactphysics3d::BodyType::STATIC;

	objectCI.objName = "base.obj";
	objectCI.textureFileName = "art/extern-textures/wood-floor.png";
	objectCI.physicsComponent = physicsComponent;
	objectCI.modelTransform = glm::translate(glm::mat4(1.f), glm::vec3(0, -5.f, 0)) *
		glm::scale(glm::mat4(1.f), glm::vec3(30.f));
	objectCI.hasPhysicsComponent = true;
	objectCI.devicePtr = c_devicePtr;
	objectCI.textureManagerPtr = m_textureManagerPtr;

	LoadObject(objectCI);

	objectCI = {};

	objectCI.modelTransform = glm::translate(glm::mat4(1.f), glm::vec3(-3.5, -1.5f, 0));
	objectCI.objName = "AnimatedCube/glTF/AnimatedCube.gltf";
	objectCI.devicePtr = c_devicePtr;
	objectCI.textureManagerPtr = m_textureManagerPtr;

	LoadObject(objectCI);

	objectCI = {};
	objectCI.modelTransform = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 1.f, 0));
	objectCI.objName = "SciFiHelmet/glTF/SciFiHelmet.gltf";
	objectCI.devicePtr = c_devicePtr;
	objectCI.textureManagerPtr = m_textureManagerPtr;

	LoadObject(objectCI);

	objectCI = {};
	objectCI.modelTransform = glm::translate(glm::mat4(1.f), glm::vec3(0.5f, -0.5f, 8)) *
		glm::scale(glm::mat4(1.f), glm::vec3(3));
	objectCI.objName = "DiffuseTransmissionTeacup/glTF/DiffuseTransmissionTeacup.gltf";
	objectCI.devicePtr = c_devicePtr;
	objectCI.textureManagerPtr = m_textureManagerPtr;

	LoadObject(objectCI);
}

void AssetManager::Update( float dt )
{
	std::shared_lock lock(m_objectMutex);
	for (auto& obj : m_objects)
	{
		Object* curr_obj = obj.second.get();
		curr_obj->Update(dt);
	}
}

const ObjectMap& AssetManager::GetObjects() const
{
	return m_objects;
}

void AssetManager::DrawObjects( const vk::DrawInfo& drawInfo ) const
{
	std::shared_lock lock(m_objectMutex);
	for (auto& obj : m_objects)
	{
		Object* curr_obj = obj.second.get();
		curr_obj->Draw(drawInfo);
	}
}
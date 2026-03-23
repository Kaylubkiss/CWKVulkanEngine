#pragma once

#include "Model.h"
#include "ResourceHandle.h"
#include <future>

class ResourceManager;
struct Scene;

struct ObjectCreateInfo
{
	//must fill out objName, even if there is no extension.
	glm::mat4 modelTransform = glm::mat4(1.0f);
	PhysicsComponent physicsComponent;
	std::string objName;
	std::string textureFilename;
	bool hasPhysicsComponent = false;
};

class Object final
{	
public:
	//Constructors
	explicit Object() = default;
	explicit Object( const ObjectCreateInfo& objectCI, ResourceManager& resourceManager );
	//Destructors
	~Object() = default;
	bool isReady()
	{
		if (pending.model.valid())
		{
			m_model = pending.model.get();
		}

		return m_model.IsValid();
	}
	//Mutators
	void LoadTextures( TextureManager& textureManager, const std::vector<std::string>& fileNames );
	void Update( const float& interpFactor );
	void InitPhysics( PhysicsSystem& physicsWorld );
	void Draw( const vk::DrawInfo& drawInfo ) const;
private:
	PhysicsComponent m_physicsComponent;
	ResourceHandle<Model> m_model;
	struct PendingInfos
	{
		std::future<ResourceHandle<Model>> model;
		std::vector<std::future<ResourceHandle<vk::Texture>>> textures;
	} pending{};

};




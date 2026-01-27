#pragma once

#include "IModel.h"
#include "TextureManager.h"
#define OBJECT_PATH "External/objects/"

struct ObjectCreateInfo
{
	//must fill out objName, even if there is no extension.
	glm::mat4 modelTransform = glm::mat4(1.0f);
	PhysicsComponent physicsComponent;
	const char* objName = "";
	const char* textureFileName = "";
	vk::Device* devicePtr = nullptr;
	bool hasPhysicsComponent = false;
};

class Object final
{	
public:
	//Constructors
	Object() = default;
	explicit Object( const ObjectCreateInfo& objectCI, TextureManager& textureManager );
	//Destructors
	~Object() = default;
	//Mutators
	void LoadTextures( TextureManager& textureManager, const std::vector<std::string>& fileNames );
	void Update( const float& interpFactor );
	void InitPhysics();
	void Draw( const vk::DrawInfo& drawInfo ) const;
private:
	std::unique_ptr<IModel> m_model;
	PhysicsComponent m_physicsComponent;
};



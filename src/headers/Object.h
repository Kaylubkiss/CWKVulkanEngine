#ifndef OBJECT_HPP
#define OBJECT_HPP

#include "Model.h"

namespace vk
{
	class TextureManager;
}

struct ObjectCreateInfo
{
	//must fill out objName, even if there is no extension.
	glm::mat4 modelTransform = glm::mat4(1.0f);
	PhysicsComponent physicsComponent;
	std::string objName;
	std::vector<std::string> textureFileNames;
	const vk::Device* devicePtr = nullptr;
	vk::TextureManager* textureManagerPtr = nullptr;
	bool hasPhysicsComponent = false;
};

class Object final
{	
public:
	//Constructors
	explicit Object() = default;
	explicit Object( const ObjectCreateInfo& objectCI, vk::TextureManager& textureManager );
	//Destructors
	~Object() = default;
	//Mutators
	void LoadTextures( vk::TextureManager& textureManager, const std::vector<std::string>& fileNames );
	void Update( const float& interpFactor );
	void InitPhysics();
	void Draw( const vk::DrawInfo& drawInfo ) const;
private:
	std::unique_ptr<Model> m_model;
	PhysicsComponent m_physicsComponent;
};

#endif


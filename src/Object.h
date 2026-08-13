#ifndef OBJECT_HPP
#define OBJECT_HPP

#include "Model.h"
#include "SceneDefinitions.h"

namespace vk
{
	class TextureManager;
}
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
	void InitPhysics( const PhysicsInitInfo& physicsInfo );
	void Draw( const vk::DrawInfo& drawInfo ) const;
private:
	std::unique_ptr<Model> m_model;
	std::optional<PhysicsComponent> m_physicsComponent;
};

#endif


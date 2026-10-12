#ifndef OBJECT_HPP
#define OBJECT_HPP

#include "Model.h"
#include "SceneDefinitions.h"

namespace vk
{
	class TextureManager;
	class DescriptorManager;
}
class Object final
{	
public:
	//Constructors
	explicit Object() = default;
	explicit Object( const ObjectCreateInfo& objectCI, vk::TextureManager& textureManager,
		vk::DescriptorManager& descriptorManager );
	//Destructors
	~Object() = default;
	//Mutators
	void LoadTextures( vk::TextureManager& textureManager, const std::vector<std::string>& fileNames );
	void UpdatePhysics( const float& interpFactor );
	void InitPhysics( const PhysicsInitInfo& physicsInfo );
	void Draw( const vk::DrawInfo& drawInfo ) const;
	void UpdateModelAnimation( const float deltaTime, const uint32_t currentFrameInFlight );
private:
	std::unique_ptr<Model> m_model;
	std::optional<PhysicsComponent> m_physicsComponent;
};

#endif


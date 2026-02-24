#pragma once
class TextureManager;

class IModel 
{
public:
	virtual ~IModel() = default;
	
	//get the bounds of the model in object space.
	[[nodiscard]] virtual glm::vec3 GetMinPoint() const = 0;
	[[nodiscard]] virtual glm::vec3 GetMaxPoint() const = 0;

	//for now, assume we only have one physics component for an entire hierarchy of meshes.\
	Obviously, there will need to be an overhaul with this. 
	virtual void UpdateModelTransform( const glm::mat4& newModelMatrix ) = 0;
	virtual void Draw( const vk::DrawInfo& drawInfo ) = 0;
	virtual void LoadTextures( TextureManager& textureManager, const std::vector<std::string>& textureNames ) = 0;
};
#ifndef SCENEDEFINITIONS_HPP
#define SCENEDEFINITIONS_HPP

#include <span>

class Camera;
class Object;

typedef std::unordered_map<std::string, std::shared_ptr<Object>> ObjectMap; //need the shared_ptr to ensure atomic reads.

struct Light
{
    glm::vec3 pos        = glm::vec3(0.f); /* position of light */
    glm::vec3 albedo     = glm::vec3(1000.f); /* base color of light */
    glm::mat4 viewMatrix = glm::mat4(1.f); /* the viewpoint of the light toward a certain point */
};

struct SceneView
{
    std::span<const std::shared_ptr<Object>> opaqueObjects;
    std::vector<std::weak_ptr<Object>> transparentObjects;
    std::span<const std::shared_ptr<Light>> lights;
    std::shared_ptr<Camera> camera;
};

struct Scene
{
    std::vector<std::shared_ptr<Object>> m_objects;
    std::vector<std::shared_ptr<Light>> m_lights;
    std::shared_ptr<Camera> m_camera;
};

// this might be consolidated with SceneInitInfo
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

struct SceneInitInfo
{
    //TODO (marked 7.20.26)
    //camera?
    //objects?
    //lights?
};

#endif
#ifndef SCENEDEFINITIONS_HPP
#define SCENEDEFINITIONS_HPP

class Object;

typedef std::unordered_map<std::string, std::shared_ptr<Object>> ObjectMap;

struct Light
{
    glm::vec3 pos        = glm::vec3(0.f); /* position of light */
    glm::vec3 albedo     = glm::vec3(1000.f); /* base color of light */
    glm::mat4 viewMatrix = glm::mat4(1.f); /* the viewpoint of the light toward a certain point */
};

struct SceneView
{
    std::vector<std::weak_ptr<Object>> opaqueObjects;
    std::vector<std::weak_ptr<Object>> transparentObjects;
    std::vector<std::weak_ptr<Light>> lights;

    void Reset()
    {
        opaqueObjects.clear();
        transparentObjects.clear();
    }
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
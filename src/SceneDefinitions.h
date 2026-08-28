#ifndef SCENEDEFINITIONS_HPP
#define SCENEDEFINITIONS_HPP

#include <span>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

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


struct PhysicsInitInfo
{
    reactphysics3d::BodyType bodyType;
    enum ColliderType
    {
        NONE = 0,
        CUBE,
        PLANE,
    };
    ColliderType colliderType = NONE;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(PhysicsInitInfo, bodyType, colliderType)

// this might be consolidated with SceneInitInfo
struct ObjectCreateInfo
{
    //must fill out objName, even if there is no extension.
    glm::mat4 modelTransform = glm::mat4(1.0f);
    std::optional<PhysicsInitInfo> physicsInfo;
    std::string objName;
    std::vector<std::string> textureFileNames;
    const vk::Device* devicePtr = nullptr;
    vk::TextureManager* textureManagerPtr = nullptr;
};

inline void to_json(json& j, const ObjectCreateInfo& objInfo)
{
    json newJson;

    std::array<std::array<float, 4>, 4> stlModelTransform;
    std::memcpy(&stlModelTransform, &objInfo.modelTransform, sizeof(stlModelTransform));

    newJson["objName"] = objInfo.objName;
    newJson["textureFileNames"] = objInfo.textureFileNames;
    if (objInfo.physicsInfo.has_value())
    {
        auto& physInit = objInfo.physicsInfo.value();
        newJson["physicsInitInfo"] = physInit;
    }

    newJson["modelTransform"] = stlModelTransform;

    j = newJson;
}

inline void from_json(const json& j, ObjectCreateInfo& objInfo)
{
    j.at("objName").get_to(objInfo.objName);
    j.at("textureFileNames").get_to(objInfo.textureFileNames);

    std::array<std::array<float, 4>, 4> stlModelTransform;
    j.at("modelTransform").get_to(stlModelTransform);
    std::memcpy(&objInfo.modelTransform, &stlModelTransform, sizeof(stlModelTransform));

    try
    {
        objInfo.physicsInfo = PhysicsInitInfo();
        j.at("physicsInitInfo").at("ColliderType").get_to(objInfo.physicsInfo->colliderType);
        j.at("physicsInitInfo").at("BodyType").get_to(objInfo.physicsInfo->bodyType);
    }
    catch (...)
    {
        std::cerr << "physicInitInfo not defined for object: " << objInfo.objName << std::endl;
    }
}

struct SceneInitInfo
{
    //TODO (marked 7.20.26)
    //camera?
    //objects?
    //lights?
};

#endif
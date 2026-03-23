#pragma once

#include "Physics.h"
#include "ResourceManager.h"
#include "ResourceHandle.h"
#include "Object.h"

struct Scene
{
    std::vector<std::shared_ptr<Object>> m_objects;
    std::vector<std::future<std::shared_ptr<Resource>>> m_pendingResources;

    void Draw( const vk::DrawInfo& drawInfo ) const
    {
        for (auto& obj : m_objects)
        {
            obj->Draw(drawInfo);
        }
    }
};

class SceneManager
{
public:
    SceneManager() = default;
    ~SceneManager() = default;

    [[nodiscard]] Scene& GetActiveScene();

    void Init( ResourceManager* resourceManager );
    void Update( float dt );
    void AddScene(/*TODO: argument storing scene data*/);
private:
    ResourceManager* m_resourceManagerPtr = nullptr;
    PhysicsSystem m_physicsWorld;
    std::vector<Scene> m_scenes;
    size_t m_activeScene = 0;

    struct ObjectData
    {
        std::shared_ptr<Object> object;
        ObjectCreateInfo objectCI = {};
    };

   // std::unordered_map<std::string, >

};
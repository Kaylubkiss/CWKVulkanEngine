#ifndef SCENEMANAGER_HPP
#define SCENEMANAGER_HPP
    //(7.20.26): this is separating what AssetManager is doing currently -> loading AND updating.
    //SceneManager:
    // - get the readied resources from asset manager (non-owning).
    // - update the resources (transforms)
    // - culling around the camera/view frustum.
    // - hand-off to renderer (sceneview)
    class SceneManager
    {
    public:
        [[nodiscard]] SceneView GetSceneView() const;

        void Init( AssetLoader& assetLoader );
        void Update( float physicsInterp, float dt ); //culling, physics
    private:
        void InitTestScene();
        void GrabRequestedObjects(); //todo: need a messaging system so asset manager can notify scenemanager instead.
    private:
        std::list<std::string> m_requestedObjects; //list of filenames the asset (resource) manager used.
        Scene m_scene; // might be useful to have a list of scenes.
        AssetLoader* c_assetLoader = nullptr;
    };

#endif
#pragma once

//TODO: finish -> replaces CameraController.h
class Controller
{
public:
    Controller() = default;
    ~Controller() = default;
    void Update();
    void SetCamera( Camera* cameraPtr );
    void SetAppSettings( AppSettings* appSettingsPtr );
private:
    Camera* m_cameraPtr = nullptr;
    AppSettings* m_appSettingsPtr = nullptr;
};
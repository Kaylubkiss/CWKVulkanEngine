#pragma once
#include "Timer.h"
#include "Camera.h"
#include "Physics.h"
#include "../vk/headers/vkContextBase.h"
#include "../vk/headers/vkInstance.h"

class Application
{
public:
	Application() = default;
	~Application();

	PhysicsSystem& GetPhysics();
	Timer& GetTimer();
	vk::ContextBase* GetVulkanContext() const;

	void run();

	void RequestExit();
	/*void SelectWorldObjects(const vk::Window& appWindow,
		Camera& camera, const uTransformObject& uTransform, PhysicsSystem& physics);*/
private:
	void init();
	void loop();
	void exit();
private:
	Timer mTime;
	PhysicsSystem m_physics;
	std::unique_ptr<vk::ContextBase> m_vulkanGraphicsContext;
	AssetManager m_assetManager;
	TextureManager m_textureManager;

	bool exitApplication = false;
};






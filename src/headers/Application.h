#pragma once
#include "Timer.h"
#include "Camera.h"
#include "Physics.h"
#include "vkContextBase.h"
#include "vkInstance.h"

class Application
{
public:
	Application() = default;
	~Application();

	PhysicsSystem& GetPhysics();

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
	bool exitApplication = false;
};






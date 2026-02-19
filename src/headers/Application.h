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

	const Timer& GetTime() const;

	PhysicsSystem& GetPhysics();
	vk::ContextBase* Context() const;

	void run();
	void RequestExit();
	/*void SelectWorldObjects(const vk::Window& appWindow,
		Camera& camera, const uTransformObject& uTransform, PhysicsSystem& physics);*/

private:
	void init();
	void loop();
	void exit();
private:
	PhysicsSystem m_physics;
	Timer mTime;

	std::unique_ptr<vk::ContextBase> m_vulkanGraphicsContext;	//this MUST be declared at the top so that it's destructor is called last.

	bool exitApplication = false;
};






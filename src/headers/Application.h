#pragma once
#include "Timer.h"
#include "Camera.h"
#include "Physics.h"
#include "ObjectManager.h"
#include "vkContextBase.h"
#include "vkCubemap.h" //TODO: remove once the implementation is done --> this is just to resolve symbol errs

class Application
{
public:
	Application() = default;
	~Application();

	const Timer& GetTime() const;

	PhysicsSystem& GetPhysics();
	vk::ContextBase* Context();

	void run();
	void RequestExit();
	void SelectWorldObjects(const vk::Window& appWindow,
		Camera& camera, const uTransformObject& uTransform, PhysicsSystem& physics);

private:
	void init();
	void loop();
	void exit();
private:
	PhysicsSystem m_physics;
	Timer mTime;

	std::unique_ptr<vk::ContextBase> m_graphicsContext;	//this MUST be declared at the top so that it's destructor is called last.

	bool exitApplication = false;
};






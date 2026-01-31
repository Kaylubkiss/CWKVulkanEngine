#pragma once
#include "Timer.h"
#include "Camera.h"
#include "Physics.h"
#include "ObjectManager.h"
#include "vkContextBase.h"

class Application
{
public:
	Application() = default;
	~Application();

	const Timer& GetTime() const;

	PhysicsSystem& GetPhysics();
	std::unique_ptr<ObjectManager>& GetObjectManager();
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
	std::unique_ptr<vk::ContextBase> m_graphicsContext;	//this MUST be declared at the top so that it's destructor is called last.
	std::unique_ptr<ObjectManager> m_objectManager;
	PhysicsSystem mPhysics;
	Timer mTime;

	bool exitApplication = false;
};






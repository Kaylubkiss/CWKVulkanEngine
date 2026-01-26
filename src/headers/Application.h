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

	const Timer& GetTime();

	PhysicsSystem& GetPhysics();
	ObjectManager& GetObjectManager();
	vk::ContextBase* Context();

	void run();
	void RequestExit();
	void SelectWorldObjects(const vk::Window& appWindow,
		Camera& camera, const uTransformObject& uTransform, PhysicsSystem& physics);
private:

	//this MUST be declared at the top so that it's destructor is called last.
	std::unique_ptr<vk::ContextBase> m_graphicsContext;
	std::unique_ptr<ObjectManager> m_objectManager;
	PhysicsSystem mPhysics;
	Timer mTime;

	bool exitApplication = false;

	void init();
	void loop();
	void exit();

};






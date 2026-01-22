#pragma once
#include "Timer.h"
#include "Camera.h"
#include "UserInterface.h"
#include "Physics.h"
#include "ObjectManager.h"
#include "vkContextBase.h"

class Application
{
private:	
	bool exitApplication = false;

public:
	Application() = default;
	~Application();

	const Timer& GetTime();

	PhysicsSystem& GetPhysics();
	vk::ObjectManager& ObjectManager();
	vk::ContextBase* Context();

	void run();
	void RequestExit();
	void SelectWorldObjects(const vk::Window& appWindow,
							Camera& camera, const uTransformObject& uTransform, PhysicsSystem& physics);
private:

	//this MUST be declared at the top so that it's destructor is called last.
	std::unique_ptr<vk::ContextBase> m_graphicsContext;
	std::unique_ptr<vk::ObjectManager> m_objectManager;
	PhysicsSystem mPhysics;
	Timer mTime;

	void init();
	void loop();
	void exit();

};






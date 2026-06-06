#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "Timer.h"
#include "Physics.h"
#include "vkRendererBase.h"
#include "TextureManager.h"
#include "DescriptorManager.h"

class Application
{
public:
	Application() = default;
	~Application();

	PhysicsSystem& GetPhysics();
	Timer& GetTimer();
	vk::RendererBase* GetVulkanRenderer() const;

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
	std::unique_ptr<vk::RendererBase> m_vulkanGraphicsContext;
	AssetManager m_assetManager;
	TextureManager m_textureManager;
	DescriptorManager m_descriptorManager;

	bool exitApplication = false;
};

#endif






#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "Timer.h"
#include "Physics.h"
#include "vkRendererBase.h"
#include "SceneManager.h"
#include "../../src-vulkan/headers/vkTextureManager.h"
#include "../../src-vulkan/headers/vkDescriptorManager.h"

class Application
{
public:
	Application() = default;
	~Application();

	PhysicsSystem& GetPhysics();
	const Timer& GetTimer() const;
	vk::RendererBase* GetVulkanRenderer() const;

	void init();
	void run();

	void RequestExit();
	/*void SelectWorldObjects(const vk::Window& appWindow,
		Camera& camera, const uTransformObject& uTransform, PhysicsSystem& physics);*/
private:
	void InitContext();
private:
	PhysicsSystem m_physics;
	Timer mTime;

	vk::Instance m_instance;
	vk::Window m_window;
	vk::Device m_device;

	std::unique_ptr<vk::RendererBase> m_vulkanGraphicsContext;

	vk::TextureManager m_textureManager;
	vk::DescriptorManager m_descriptorManager;

	//unfortunately, these rely on the devices to be alive.
	AssetLoader m_assetManager;
	SceneManager m_sceneManager;

	bool exitApplication = false;
};

extern Application app;

#endif






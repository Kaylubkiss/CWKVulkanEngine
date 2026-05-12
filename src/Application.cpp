#include "Input.h"
#include "vkDeferredRenderer.h"

#include "test.h"

PhysicsSystem& Application::GetPhysics() 
{
	return this->m_physics;
}

Timer& Application::GetTimer()
{
	return this->mTime;
}

vk::RendererBase* Application::GetVulkanRenderer() const
{
	return m_vulkanGraphicsContext.get();
}

void Application::run()
{
	//initialize all resources.
	Application::init();

	//Application::test();
	 
	//render, update, render, update...
	Application::loop();

	//cleanup resources
	Application::exit();
}


void Application::init() 
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
	}

	m_vulkanGraphicsContext = std::make_unique<vk::DeferredRenderer>(&m_textureManager, &m_descriptorManager);

	if (exitApplication == true)
	{
		return;
	}

	auto rendererInfo = m_vulkanGraphicsContext->GetInfo();

	m_assetManager.Init(rendererInfo.devicePtr, &m_textureManager, 2);
}

void Application::test()
{
	auto rendererInfo = m_vulkanGraphicsContext->GetInfo();

	std::mutex mootMutex;
	std::vector<std::string> fileNames = {"art/extern-textures/monochrome_studio.hdr"};
	test::LoadPanoramicImage(rendererInfo.devicePtr, fileNames,mootMutex);

	RequestExit();
}


/*void Application::SelectWorldObjects(const vk::Window& appWindow,
									 Camera& camera, const uTransformObject& uTransform, PhysicsSystem& physics)
{
	
	int mouseX = 0, mouseY = 0;

	if (SDL_GetRelativeMouseMode() == SDL_FALSE) 
	{
		SDL_GetMouseState(&mouseX, &mouseY);
	}
	else 
	{
		mouseX = appWindow.center_x;
		mouseY = appWindow.center_y;
	}

	glm::vec4 cursorWindowPos(mouseX, mouseY, 1, 1);

	glm::vec4 cursorScreenPos = {};

	//ndc
	cursorScreenPos.x = (2 * cursorWindowPos.x) / appWindow.viewport.width - 1;
	cursorScreenPos.y = (2 * cursorWindowPos.y) / appWindow.viewport.height - 1; //vulkan is upside down.
	cursorScreenPos.z = 1;
	cursorScreenPos.w = 1;

	////eye

	////world 
	glm::vec4 ray_world_far = glm::inverse(uTransform.proj * uTransform.view) * cursorScreenPos;

	ray_world_far /= ray_world_far.w;

	cursorScreenPos.z = 0;
	glm::vec4 ray_world_near = glm::inverse(uTransform.proj * uTransform.view) * cursorScreenPos;

	ray_world_near /= ray_world_near.w;

	//2. cast ray from the mouse position and in the direction forward from the mouse position

	reactphysics3d::Vector3 rayStart(ray_world_near.x, ray_world_near.y, ray_world_near.z);

	reactphysics3d::Vector3 rayEnd(ray_world_far.x, ray_world_far.y, ray_world_far.z);

	Ray ray(rayStart, rayEnd);

	RaycastInfo raycastInfo = {};

	RayCastObject callbackObject;

	physics.World()->raycast(ray, &callbackObject);

}*/

void Application::RequestExit() 
{
	this->exitApplication = true;
}

void Application::loop()
{
	if (m_vulkanGraphicsContext != nullptr)
	{
		//render graphics.
		while (exitApplication == false)
		{
			double realFrameTime = mTime.CalculateDeltaTime();

			float physicsTime = m_physics.InterpFactor(static_cast<float>(realFrameTime));

			Input::MoveCamera(m_vulkanGraphicsContext->GetCamera(), static_cast<float>(realFrameTime));

			if (exitApplication)
			{
				break;
			}

			m_assetManager.Update(physicsTime);

			m_vulkanGraphicsContext->Render(m_assetManager);
		}

		//when we're done with the loop, we should make sure the logical device is flushed.
		m_vulkanGraphicsContext->WaitForDevice();
	}
}


void Application::exit()
{
	SDL_Quit();

	m_assetManager.Destroy();
	m_textureManager.Destroy();
	m_descriptorManager.Destroy();
}


Application::~Application()
{
}


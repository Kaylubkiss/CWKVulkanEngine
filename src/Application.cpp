#include "CameraController.h"
#include "vk-scenes/vkDeferredShadingContext.h"

PhysicsSystem& Application::GetPhysics() 
{
	return this->m_physics;
}

vk::ContextBase* Application::GetVulkanContext() const
{
	return m_vulkanGraphicsContext.get();
}


void Application::run() 
{
	//initialize all resources.
	Application::init();
	 
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

	m_vulkanGraphicsContext = std::make_unique<vk::DeferredContext>();

	if (exitApplication == false)
	{
		m_vulkanGraphicsContext->InitializeScene(); //TODO: deserialize a scene
	}
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
			float dt = m_physics.InterpFactor(static_cast<float>(mTime.CalculateDeltaTime()));

			Controller::MoveCamera(m_vulkanGraphicsContext->GetCamera(), dt);

			m_vulkanGraphicsContext->UpdateSceneObjects(dt);

			m_vulkanGraphicsContext->Render();
		}

		//when we're done with the loop, we should make sure the logical device is flushed.
		m_vulkanGraphicsContext->WaitForDevice();
	}
}


void Application::exit()
{
	SDL_Quit();
}


Application::~Application()
{
}


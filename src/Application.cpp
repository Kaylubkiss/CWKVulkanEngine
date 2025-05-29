#include "Application.h"
#include <iostream>
#include <SDL2/SDL_vulkan.h>
#include "vkDebug.h"
#include "vkInit.h"
#include "Controller.h"
#include "Physics.h"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/gtc/matrix_transform.hpp>


//NOTE: to remove pesky warnings from visual studio, on dynamically allocated arrays,
//I've used the syntax: *(array + i) to access the array instead of array[i].
//the static analyzer of visual studio is bad.


Camera& Application::GetCamera()
{
	return this->mCamera;
}

PhysicsSystem& Application::GetPhysics() 
{
	return this->mPhysics;
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


void Application::CreateWindow(vk::Window& appWindow)
{

	//set the viewport depth value!!!
	appWindow.viewport.width = 640;
	appWindow.viewport.height = 480;
	appWindow.viewport.minDepth = 0;
	appWindow.viewport.maxDepth = 1;

	appWindow.scissor.extent.width = (uint32_t)this->mWindow.viewport.width;
	appWindow.scissor.extent.height = (uint32_t)this->mWindow.viewport.height;

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
	}

	appWindow.sdl_ptr = SDL_CreateWindow("Caleb's Vulkan Engine", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
	int(appWindow.viewport.width), int(appWindow.viewport.height), SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

	if (appWindow.sdl_ptr == nullptr)
	{
		printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
	}

}

bool Application::WindowisFocused() 
{
	if (this->mWindow.sdl_ptr == NULL) 
	{
		return false;
	}

	uint32_t flags = SDL_GetWindowFlags(this->mWindow.sdl_ptr);

	return ((flags & SDL_WINDOW_INPUT_FOCUS) != 0);

}

void Application::CreateWindowSurface(const VkInstance& vkInstance, vk::Window& appWindow) 
{
	if (SDL_Vulkan_CreateSurface(appWindow.sdl_ptr, vkInstance, &appWindow.surface) != SDL_TRUE)
	{
		throw std::runtime_error("could not create window surface! " + std::string(SDL_GetError()));
	}
}

void Application::InitGui() 
{
	//IMGUI_CHECKVERSION();
	//ImGui::CreateContext();

	//ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NoMouseCursorChange;

	//// Setup Platform/Renderer backends
	//if (!ImGui_ImplSDL2_InitForVulkan(this->mWindow.sdl_ptr)) {

	//	throw std::runtime_error("couldn't initialize imgui for vulkan!!!\n");
	//	return;
	//}


	//ImGui_ImplVulkan_InitInfo init_info = {};
	//init_info.Instance = this->m_instance;
	//init_info.PhysicalDevice = mGraphicsSystem.PhysicalDevice();
	//init_info.Device = mGraphicsSystem.LogicalDevice();
	//init_info.QueueFamily = mGraphicsSystem.GraphicsQueue().family;
	//init_info.Queue = mGraphicsSystem.GraphicsQueue().handle;
	//init_info.PipelineCache = VK_NULL_HANDLE;
	//init_info.DescriptorPool = mGraphicsU;
	//init_info.RenderPass = this->m_renderPass;
	//init_info.Subpass = 0;
	//init_info.MinImageCount = 2;
	//init_info.ImageCount = this->imageCount;
	//init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	//init_info.Allocator = nullptr;
	//init_info.CheckVkResultFn = vk::util::check_vk_result;
	//ImGui_ImplVulkan_Init(&init_info);
}



bool Application::init() 
{
	this->mCamera = Camera({ 0.f, 0.f, 10.f }, { 0.f, 0.f, -1.f } , { 0,1,0 });

	CreateWindow(this->mWindow);

	this->m_instance = vk::init::CreateInstance(this->mWindow.sdl_ptr);
	
	CreateWindowSurface(this->m_instance, this->mWindow);

	this->mGraphicsSystem = vk::GraphicsSystem(this->m_instance, this->mWindow);

	this->mGraphicsSystem.AttachHotReloader(this->mHotReloader);

	this->secondaryCmdBuffer = vk::init::CommandBuffer(this->mGraphicsSystem.LogicalDevice(), this->mGraphicsSystem.CommandPool(), VK_COMMAND_BUFFER_LEVEL_SECONDARY);

	this->mTextureManager.Init(this->mGraphicsSystem.LogicalDevice());
	
	this->mObjectManager.Init();
	this->mObjectManager.AttachSystems(&this->mTextureManager, &this->mGraphicsSystem);
	
	glm::mat4 modelTransform = glm::mat4(5.f);
	modelTransform[3] = glm::vec4(1.0f, 0, 5.f, 1);

	mObjectManager.LoadObject(mGraphicsSystem.PhysicalDevice(), mGraphicsSystem.LogicalDevice(), "freddy.obj", modelTransform, "texture.jpg", nullptr, false, "freddy");

	//object 2
	modelTransform = glm::mat4(1.f);
	modelTransform[3] = glm::vec4(0, 20, -5.f, 1);

	PhysicsComponent physicsComponent;
	physicsComponent.bodyType = BodyType::DYNAMIC;
	physicsComponent.colliderType = PhysicsComponent::ColliderType::CUBE;

	mObjectManager.LoadObject(mGraphicsSystem.PhysicalDevice(), mGraphicsSystem.LogicalDevice(), "cube.obj", modelTransform, "puppy1.bmp", &physicsComponent, true, "cube");

	//object 3
	const float dbScale = 30.f;
	modelTransform = glm::mat4(dbScale);
	modelTransform[3] = { 0.f, -5.f, 0.f, 1 };
	
	physicsComponent.bodyType = reactphysics3d::BodyType::STATIC;
	mObjectManager.LoadObject(mGraphicsSystem.PhysicalDevice(), mGraphicsSystem.LogicalDevice(), "base.obj", modelTransform, "puppy1.bmp", &physicsComponent, true, "base");
	
	//InitGui();

	mPhysics.Init();

	/*VkDebugUtilsObjectNameInfoEXT dscPoolDebug = {};
	dscPoolDebug.objectHandle = (uint64_t)(this->mTextureManager.DescriptorPool());
	dscPoolDebug.objectType = VK_OBJECT_TYPE_DESCRIPTOR_POOL;
	dscPoolDebug.pObjectName = "Texture Manager Descriptor Pool";


	auto* SetDebugObjectName = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetInstanceProcAddr(this->m_instance, "vkSetDebugUtilsObjectNameEXT"));

	VK_CHECK_RESULT(SetDebugObjectName(this->mGraphicsSystem.LogicalDevice(), &dscPoolDebug))*/

	mTime = Timer(SDL_GetPerformanceCounter());

	return true;


}

vk::Window& Application::GetWindow()
{
	return this->mWindow;
}

const Timer& Application::GetTime()
{
	return this->mTime;
}


void Application::SelectWorldObjects(const vk::Window& appWindow, 
									 Camera& camera, const vk::uTransformObject& uTransform, PhysicsSystem& physics)
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

}

void Application::DrawGui(VkCommandBuffer cmdBuffer)
{

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();

	const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + main_viewport->WorkSize.x / 15, main_viewport->WorkPos.y + main_viewport->WorkSize.y / 10), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(main_viewport->WorkSize.x / 3, main_viewport->WorkSize.y / 2), ImGuiCond_Once);


	ImGuiWindowFlags window_flags = 0;
	window_flags |= ImGuiWindowFlags_MenuBar;
	// Main body of the Demo window starts here.
	if (!ImGui::Begin("Asset Log", nullptr, window_flags))
	{
		// Early out if the window is collapsed, as an optimization
		this->guiWindowIsFocused = ImGui::IsWindowFocused();
		ImGui::End();
		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);
		return;
	}

	this->guiWindowIsFocused = ImGui::IsWindowFocused();
	
	ImGui::End();
	ImGui::Render();
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);


}

void Application::RequestExit() 
{
	this->exitApplication = true;
}


void Application::loop()
{
	//render graphics.
	while (exitApplication == false)
	{	
		mTime.Update();

		bool result = Controller::MoveCamera(mCamera, mTime.DeltaTime());
		if (result) { mGraphicsSystem.UpdateUniformViewMatrix(mCamera.LookAt()); }
		/*SelectWorldObjects(this->mWindow, this->mCamera, vk::global::uTransform, this->mPhysics);*/

		mPhysics.Update(mTime.DeltaTime());

		mGraphicsSystem.WaitForQueueSubmission();

		mHotReloader.HotReload();

		//draw objects using secondary command buffer
		VkCommandBufferInheritanceInfo inheritanceInfo = {};
		inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
		inheritanceInfo.renderPass = mGraphicsSystem.RenderPass();
		inheritanceInfo.subpass = 0;

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
		beginInfo.pInheritanceInfo = &inheritanceInfo;

		VK_CHECK_RESULT(vkBeginCommandBuffer(this->secondaryCmdBuffer, &beginInfo))
		
		vkCmdBindPipeline(this->secondaryCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->mGraphicsSystem.Pipeline());

		vkCmdSetViewport(this->secondaryCmdBuffer, 0, 1, &mWindow.viewport);
		vkCmdSetScissor(this->secondaryCmdBuffer, 0, 1, &mWindow.scissor);

		this->mObjectManager.Update(mPhysics.InterpFactor(), this->secondaryCmdBuffer);

		VK_CHECK_RESULT(vkEndCommandBuffer(this->secondaryCmdBuffer))

		//sync this up with primary command buffer in graphics system...
		mGraphicsSystem.Render(this->mWindow, &this->secondaryCmdBuffer, 1);	
	}
	
	//when we're done with the loop, we should make sure the logical device is flushed.
	mGraphicsSystem.WaitForDevice();

}


void Application::exit()
{
	mTextureManager.Destroy(mGraphicsSystem.LogicalDevice());
	mObjectManager.Destroy(mGraphicsSystem.LogicalDevice());

	mGraphicsSystem.Destroy();
}


Application::~Application()
{

	if (this->m_instance != VK_NULL_HANDLE) 
	{
		if (this->mWindow.surface != nullptr) 
		{
			vkDestroySurfaceKHR(this->m_instance, this->mWindow.surface, nullptr);
		}

		vkDestroyInstance(this->m_instance, nullptr);
	}
}


void Application::CleanUpGui() 
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
}






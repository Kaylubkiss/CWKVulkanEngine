#include "Application.h"
#include <iostream>
#include <SDL2/SDL_vulkan.h>
#include <glm/gtc/matrix_transform.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include "vkUtility.h"
#include "vkDebug.h"
#include "vkInit.h"
#include "vkResource.h"


//NOTE: to remove pesky warnings from visual studio, on dynamically allocated arrays,
//I've used the syntax: *(array + i) to access the array instead of array[i].
//the static analyzer of visual studio is bad.


static const std::string shaderPath{ "Shaders/" };

const static glm::vec4 X_BASIS = { 1,0,0,0 };
const static glm::vec4 Y_BASIS = { 0,1,0,0 };
const static glm::vec4 Z_BASIS = { 0,0,1,0 };
const static glm::vec4 W_BASIS = { 0,0,0,1 };

void Application::UpdateUniformViewMatrix() 
{
	if (mCamera.isUpdated()) 
	{
		mGraphicsSystem.UpdateUniformViewMatirx(mCamera.LookAt());
	}
}

Camera& Application::GetCamera() const
{
	return this->mCamera;
}

void Application::run() 
{
	//initialize all resources.
	init();

	//render, update, render, update...
	loop();

	//cleanup resources
	exit();
}

void Application::ToggleObjectVisibility(SDL_Keycode keysym, uint8_t lshift) 
{
	/*debugCube3.debugDrawObject.ToggleVisibility(keysym, lshift);
	debugCube2.debugDrawObject.ToggleVisibility(keysym, lshift);*/

}

void Application::CreateWindow(vk::Window& appWindow)
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
	}

	appWindow.sdl_ptr = SDL_CreateWindow("Caleb's Vulkan Engine", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
	int(appWindow.viewport.width), int(appWindow.viewport.height), SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

	if (appWindow.sdl_ptr == NULL)
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
		throw std::runtime_error("could not create window surface!");
	}
}



const VkPipeline& Application::GetLinePipeline()
{
	return this->linePipeline;
}

void Application::CreateDescriptorSets()
{
	//create descriptor pool
	VkDescriptorPoolSize poolSize[2] = {};
	poolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize[0].descriptorCount = 2; //max numbers of frames in flight.

	//we are concerned about the fragment stage, so we double the descriptor count here.
	poolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSize[1].descriptorCount = 1 * 2; //max numbers of frames in flight times two to accomodate the gui.

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.poolSizeCount = (uint32_t)2;
	poolInfo.pPoolSizes = poolSize;
	poolInfo.maxSets = mTextures.size() > 0? mTextures.size() * 2 : 1; //max numbers of frames in flight.

	VK_CHECK_RESULT(vkCreateDescriptorPool(this->m_logicalDevice, &poolInfo, nullptr, &this->descriptorPool));

	for (size_t i = 0; i < mTextures.size(); ++i) 
	{
		VkDescriptorSetAllocateInfo descriptorAllocInfo{};
		descriptorAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorAllocInfo.descriptorPool = this->descriptorPool;
		descriptorAllocInfo.descriptorSetCount = 1;
		descriptorAllocInfo.pSetLayouts = &this->descriptorSetLayout;

		VK_CHECK_RESULT(vkAllocateDescriptorSets(this->m_logicalDevice, &descriptorAllocInfo, &this->mTextures[i].mDescriptor));

	}
}

void Application::WriteDescriptorSets() 
{
	VkDescriptorBufferInfo uTransformbufferInfo = {};
	uTransformbufferInfo.buffer = uniformBuffers[0].handle;
	uTransformbufferInfo.offset = 0;
	uTransformbufferInfo.range = sizeof(uTransformObject);

	VkDescriptorBufferInfo uLightInfoBufferInfo = {};
	uLightInfoBufferInfo.buffer = mLights.mBuffer.handle;
	uLightInfoBufferInfo.offset = 0;
	uLightInfoBufferInfo.range = sizeof(LightInfoObject) - sizeof(int) * LightCountIndex::MAX_IND_COUNT;

	VkDescriptorBufferInfo bufferInfo[2] = { uTransformbufferInfo, uLightInfoBufferInfo };
	VkDescriptorImageInfo imageInfo = {};

	for (size_t i = 0; i < this->mTextures.size(); ++i) 
	{
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = this->mTextures[i].mTextureImageView;
		imageInfo.sampler = this->mTextures[i].mTextureSampler;

		VkWriteDescriptorSet descriptorWrite[3] = {};
		descriptorWrite[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite[0].dstSet = this->mTextures[i].mDescriptor;
		descriptorWrite[0].dstBinding = 0;
		descriptorWrite[0].dstArrayElement = 0;
		descriptorWrite[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite[0].descriptorCount = 1; //how many buffers
		descriptorWrite[0].pBufferInfo = &uTransformbufferInfo;
		descriptorWrite[0].pImageInfo = nullptr; // Optional
		descriptorWrite[0].pTexelBufferView = nullptr; // Optional

		descriptorWrite[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite[1].dstSet = this->mTextures[i].mDescriptor;
		descriptorWrite[1].dstBinding = 1;
		descriptorWrite[1].dstArrayElement = 0;
		descriptorWrite[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite[1].descriptorCount = 1; //how many buffers
		descriptorWrite[1].pBufferInfo = &uLightInfoBufferInfo;
		descriptorWrite[1].pImageInfo = nullptr; // Optional
		descriptorWrite[1].pTexelBufferView = nullptr; // Optional

		descriptorWrite[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite[2].dstSet = this->mTextures[i].mDescriptor;
		descriptorWrite[2].dstBinding = 2;
		descriptorWrite[2].dstArrayElement = 0;
		descriptorWrite[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrite[2].descriptorCount = 1; //how many images
		descriptorWrite[2].pImageInfo = &imageInfo;
		descriptorWrite[2].pTexelBufferView = nullptr; // Optional

		vkUpdateDescriptorSets(this->m_logicalDevice, 3, descriptorWrite, 0, nullptr);
	}

}

void Application::CreateFences(const VkDevice l_device) 
{
	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; //to prevent indefinite waiting on first frame.

	VK_CHECK_RESULT(vkCreateFence(l_device, &fenceInfo, nullptr, &this->inFlightFence));
}


void Application::InitPhysicsWorld() 
{
	this->mObjectManager["cube"].InitPhysics(ColliderType::CUBE);
	
	reactphysics3d::Material& db2Material = this->mObjectManager["cube"].mPhysicsComponent.collider->getMaterial();
	db2Material.setBounciness(0.f);
	db2Material.setMassDensity(10.f);
	this->mObjectManager["cube"].mPhysicsComponent.rigidBody->updateMassPropertiesFromColliders();
	
	this->mObjectManager["base"].InitPhysics(ColliderType::CUBE, BodyType::STATIC);
	
	mCamera.InitPhysics(BodyType::STATIC);


	this->mObjectManager["cube"].SetLinesArrayOffset(12);

	//this->mPhysicsWorld->setIsDebugRenderingEnabled(true);
	this->mPhysics.GetPhysicsWorld()->setIsDebugRenderingEnabled(true);

	this->mObjectManager["cube"].mPhysicsComponent.rigidBody->setIsDebugEnabled(true);
	this->mObjectManager["cube"].mPhysicsComponent.rigidBody->setIsDebugEnabled(true);
	
	//the order they were added to the physics world
	reactphysics3d::DebugRenderer& debugRenderer = this->mPhysics.GetPhysicsWorld()->getDebugRenderer();
	debugRenderer.setIsDebugItemDisplayed(reactphysics3d::DebugRenderer::DebugItem::COLLIDER_AABB, true);
	
}

void Application::InitGui() 
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NoMouseCursorChange;

	// Setup Platform/Renderer backends
	if (!ImGui_ImplSDL2_InitForVulkan(this->window)) {

		throw std::runtime_error("couldn't initialize imgui for vulkan!!!\n");
		return;
	}


	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = this->m_instance;
	init_info.PhysicalDevice = this->m_physicalDevices[device_index];
	init_info.Device = this->m_logicalDevice;
	init_info.QueueFamily = this->graphicsFamily;
	init_info.Queue = this->graphicsQueue;
	init_info.PipelineCache = VK_NULL_HANDLE;
	init_info.DescriptorPool = this->descriptorPool;
	init_info.RenderPass = this->m_renderPass;
	init_info.Subpass = 0;
	init_info.MinImageCount = 2;
	init_info.ImageCount = this->imageCount;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.Allocator = nullptr;
	init_info.CheckVkResultFn = vk::util::check_vk_result;
	ImGui_ImplVulkan_Init(&init_info);
}


bool Application::init() 
{
	this->mCamera = Camera({ 0.f, 0.f, 10.f }, { 0.f, 0.f, -1.f } , { 0,1,0 });

	this->mWindow.viewport.width = 640;
	this->mWindow.viewport.height = 480;

	CreateWindow(this->mWindow);

	m_instance = vk::init::CreateInstance(this->mWindow.sdl_ptr);

	CreateWindowSurface(this->m_instance, this->mWindow);

	//setup the debug callbacks... (optional...)

	this->mGraphicsSystem = vk::GraphicsSystem(this->m_instance, this->mWindow);
	
	//retrieve queue family properties 
	// --> group of queues that have identical capabilities and are able to run in parallel 
	//		--> could be arithmetic, passing shaders, stuff like that.

	glm::mat4 modelTransform = glm::mat4(5.f);
	modelTransform[3] = glm::vec4(1.f, 0, -20.f, 1);

	mObjectManager.LoadObject("freddy", "freddy.obj", false, modelTransform);

	//object 2
	modelTransform = glm::mat4(1.f);
	modelTransform[3] = glm::vec4(0, 20, -5.f, 1);

	mObjectManager.LoadObject("cube", "cube.obj", true, modelTransform);

	//object 3
	const float dbScale = 30.f;
	modelTransform = glm::mat4(dbScale);
	modelTransform[3] = { 0.f, -5.f, 0.f, 1 };

	mObjectManager.LoadObject("base", "base.obj", true, modelTransform);

	
	// If you want to draw a triangle:
	// - create renderpass object
	CreateSwapChain();

	CreateImageViews();

	std::string vertexShaderPath = shaderPath + "blinnvert.spv";
	VkPipelineShaderStageCreateInfo shaderVertStageInfo = vk::util::CreateShaderModule(this->m_logicalDevice, vertexShaderPath.data(), this->shaderVertModule, VK_SHADER_STAGE_VERTEX_BIT);

	std::string fragShaderPath = shaderPath + "blinnfrag.spv";
	VkPipelineShaderStageCreateInfo shaderFragModuleInfo = vk::util::CreateShaderModule(this->m_logicalDevice, fragShaderPath.data(), this->shaderFragModule, VK_SHADER_STAGE_FRAGMENT_BIT);

	VkPipelineShaderStageCreateInfo shaderStages[] = { shaderVertStageInfo, shaderFragModuleInfo };

	////create layout 
	this->commandPool = vk::init::CreateCommandPool(this->m_logicalDevice, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	this->commandBuffer = CreateCommandBuffer(this->m_logicalDevice, this->commandPool);
	
	CreateUniformBuffers();


	mLights.Create({ 0, 10, 0 }, { 0, -1, 0 });
	
	CreateDepthResources();

	//ERROR: didn't create descriptoryLayout!!!

	CreatePipelineLayout();
	
	CreateRenderPass();
	CreateFrameBuffers();
	
	CreatePipeline(shaderStages, 2, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, this->pipeline);
	CreatePipeline(shaderStages, 2, VK_PRIMITIVE_TOPOLOGY_LINE_LIST, this->linePipeline);

	//commands


	CreateSemaphores();

	CreateFences();

	
	mTime = Time(SDL_GetPerformanceCounter());

	while (mObjectManager.mThreadWorkers.isBusy()) { //wait until the jobs are done... 
	}

	this->mObjectManager["freddy"].UpdateTexture("texture.jpg");
	this->mObjectManager["freddy"].UpdatePipelineLayout(&this->pipelineLayouts.back());

	this->mObjectManager["cube"].UpdateTexture("puppy1.bmp");
	this->mObjectManager["cube"].UpdatePipelineLayout(&this->pipelineLayouts.back());

	this->mObjectManager["base"].UpdateTexture("puppy1.bmp");
	this->mObjectManager["base"].UpdatePipelineLayout(&this->pipelineLayouts.back());


	CreateDescriptorSets();
	WriteDescriptorSets();


	InitGui();

	InitPhysicsWorld();

	//ERROR: vksetcmdviewport? scissor? you made a dynamic viewport!!!

	return true;


}


vk::Window& Application::GetWindow() const
{
	return this->mWindow;
}

const Time& Application::GetTime()
{
	return this->mTime;
}

class RayCastObject : public RaycastCallback {
public:
	virtual decimal notifyRaycastHit(const RaycastInfo& info)
	{
		// Display the world hit point coordinates
		std::cout << " Hit point : " <<
			info.worldPoint.x <<
			info.worldPoint.y <<
			info.worldPoint.z <<
			std::endl;

		// Return a fraction of 1.0 to gather all hits
		return decimal(-1.0);
	}
};


//void Application::SelectWorldObjects(const int& mouseX, const int& mouseY)
//{
//	
//
//	glm::vec4 cursorWindowPos(mouseX, mouseY, 1, 1);
//
//	glm::vec4 cursorScreenPos = {};
//
//	//ndc
//	cursorScreenPos.x = (2 * cursorWindowPos.x) / this->mWindowExtents.width - 1;
//	cursorScreenPos.y = 1 - (2 * cursorWindowPos.y) / this->mWindowExtents.height; //vulkan is upside down.
//	cursorScreenPos.z = -1;
//	cursorScreenPos.w = 1;
//
//	////eye
//
//	////world 
//	glm::vec4 ray_world = glm::inverse(uTransform.view * uTransform.proj) * cursorScreenPos;
//
//	ray_world /= ray_world.w;
//
//	//2. cast ray from the mouse position and in the direction forward from the mouse position
//
//	glm::vec3 CameraPos = mCamera.Position();
//
//	reactphysics3d::Vector3 rayStart(CameraPos.x, CameraPos.y, CameraPos.z);
//
//	reactphysics3d::Vector3 rayEnd(ray_world.x, ray_world.y, ray_world.z);
//
//	Ray ray(rayStart, rayEnd);
//
//	RaycastInfo raycastInfo = {};
//
//	RayCastObject callbackObject;
//
//	this->mPhysics.GetPhysicsWorld()->raycast(ray, &callbackObject);
//
//}

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

		mController.Update();

		mPhysics.Update(mTime.DeltaTime());

		this->mObjectManager["cube"].Update(mPhysics.InterpFactor());
		this->mObjectManager["base"].Update(mPhysics.InterpFactor());
		
		mCamera.Update(mPhysics.InterpFactor());
		Application::UpdateUniformViewMatrix();

		mLights.Update();
		
		//rendering objects
		mGraphicsSystem.Render();

		
	}

	VK_CHECK_RESULT(vkDeviceWaitIdle(this->m_logicalDevice));



}


void Application::exit()
{

	CleanUpGui();

	//vkDestroyPipeline(this->m_logicalDevice, this->linePipeline, nullptr);

	vkDestroyDescriptorPool(this->m_logicalDevice, this->descriptorPool, nullptr);

	vkDestroyDescriptorSetLayout(this->m_logicalDevice, this->descriptorSetLayout, nullptr);
	
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(this->m_instance, "vkDestroyDebugUtilsMessengerEXT");

	if (func != nullptr) 
	{
		func(this->m_instance, this->debugMessenger, nullptr);
	}

	mLights.Deallocate();

	/*mObjectManager.Deallocate();*/

}


Application::~Application()
{
	vkDestroySurfaceKHR(this->m_instance, this->mWindow.surface, nullptr);

	vkDestroyInstance(this->m_instance, nullptr);
	
}


void Application::CleanUpGui() 
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
}






#include "Application.h"
#include <iostream>
#include <cassert>
#include <fstream>
#include <SDL2/SDL_vulkan.h>
#include <glm/gtc/matrix_transform.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <imgui/imgui_demo.cpp>


//NOTE: to remove pesky warnings from visual studio, on dynamically allocated arrays,
//I've used the syntax: *(array + i) to access the array instead of array[i].
//the static analyzer of visual studio is bad.

static unsigned long long width = 640;
static unsigned long long height = 480;

static bool guiWindowIsFocused = false;

#define VK_CHECK_RESULT(function) {VkResult check = function; assert(check == VK_SUCCESS); if (check != VK_SUCCESS) {std::cout << check << std::endl;}}

static const int const_textureCount = 3;

static void check_vk_result(VkResult err)
{
	if (err == 0)
		return;
	fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
	if (err < 0)
		abort();
}

static glm::vec4 X_BASIS = { 1,0,0,0 };
static glm::vec4 Y_BASIS = { 0,1,0,0 };
static glm::vec4 Z_BASIS = { 0,0,1,0 };
static glm::vec4 W_BASIS = { 0,0,0,1 };

struct uTransformObject 
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};


static uTransformObject uTransform =
{
	glm::mat4(1.), //model
	glm::lookAt(glm::vec3(0.f, 0.f , 50.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f,1.f,0.f)), //view
	glm::perspective(glm::radians(45.f), (float)width/height,  0.1f, 1000.f) //proj
};


static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
VkDebugUtilsMessageTypeFlagsEXT messageType,
const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
void* pUserData) 
{
	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) 
	{
		std::cerr << pCallbackData->pMessage << std::endl << std::endl;
	}

	return VK_FALSE;
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

void Application::FillDebugMessenger(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = 
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

	createInfo.pfnUserCallback = debugCallback;

}


void Application::CreateInstance() 
{

	//create instance info.
	VkInstanceCreateInfo createInfo = {};
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.apiVersion = VK_API_VERSION_1_3;
	appInfo.pApplicationName = "Caleb Vulkan Engine";
	appInfo.engineVersion = 1;

	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = nullptr; //best practice is to fill out this info, but we won't for now.
	createInfo.flags = 0;

	//linked list of structures to pass to the create instance func.
	//--> look into it later.
	createInfo.pNext = nullptr;


	//we won't be doing any extension for now --> look into it at a later time.
	//need to get sdl extensionss
	unsigned int extensionCount = 0;
	const char** extensionNames;

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
	FillDebugMessenger(debugCreateInfo);

	if (SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr) != SDL_TRUE)
	{
		throw std::runtime_error("could not grab extensions from SDL!");
	}

	extensionNames = new const char* [extensionCount + 1];

	if (SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, (extensionNames)) != SDL_TRUE)
	{
		throw std::runtime_error("could not grab extensions from SDL!");
	}


	extensionNames[extensionCount] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
	
	createInfo.enabledExtensionCount = extensionCount + 1;
	createInfo.ppEnabledExtensionNames = (extensionNames);


	//this could be useful for logging, profiling, debugging, whatever.
	//it intercepts the API

	if (CheckValidationSupport())
	{
		createInfo.ppEnabledLayerNames = enabledLayerNames;
		createInfo.enabledLayerCount = 1;
	}

	createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	createInfo.pApplicationInfo = &appInfo;

	//create instance.
	//this function, if successful, will create a "handle object"
	//and make pInstance the handle. A handle is always 64-bits wide.  

	//also, setting the pAllocator to null will make vulkan do its
	//own memory management, whereas we can create our own allocator
	//for vulkan to use
	VK_CHECK_RESULT(vkCreateInstance(&createInfo, nullptr, &this->m_instance));

	delete [] extensionNames;


	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(this->m_instance, "vkCreateDebugUtilsMessengerEXT");

	if (func != nullptr) {

		func(this->m_instance, &debugCreateInfo, nullptr, &this->debugMessenger);
	}
	else 
	{
		throw std::runtime_error("could not set up debug messenger");

	}

}

bool Application::CheckValidationSupport() 
{
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);

	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	
	for (const char* layerName : enabledLayerNames) 
	{
		bool layerFound = false;
		for (const VkLayerProperties& layerProperties : availableLayers) 
		{
			if (strcmp(layerName, layerProperties.layerName) == 0) 
			{
				layerFound = true;
				break;
			}
		}

		if (layerFound == false) 
		{
			return false;
		}

	}


	return true;

}

void Application::EnumeratePhysicalDevices() 
{
	//list the physical devices
	uint32_t max_devices = 0;

	//vulkan will ignor whatever was set in physicalDeviceCount and overwrite max_devices 
	VK_CHECK_RESULT(vkEnumeratePhysicalDevices(this->m_instance, &max_devices, nullptr));

	if (max_devices <= 0)
	{
		throw std::runtime_error("could not find any GPUs to use!\n");
		return;
	}


	this->m_physicalDevices = new VkPhysicalDevice[max_devices];

	if (this->m_physicalDevices == NULL) 
	{
		throw std::runtime_error("could not allocate array of physical devices\n");
	}

	VK_CHECK_RESULT(vkEnumeratePhysicalDevices(this->m_instance, &max_devices, this->m_physicalDevices));

	for (size_t i = 0; i < max_devices; ++i)
	{
		VkPhysicalDeviceProperties properties;
		VkPhysicalDeviceFeatures features;

		vkGetPhysicalDeviceProperties(*(this->m_physicalDevices + i), &properties);
		vkGetPhysicalDeviceFeatures(*(this->m_physicalDevices + i), &features);

		if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && features.geometryShader && features.samplerAnisotropy)
		{
			device_index = i;
		}
	}

	if (device_index < 0)
	{
		throw std::runtime_error("could not find suitable physical device!");
	}


}

void Application::CreateWindow() 
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
	}

	this->window = SDL_CreateWindow("Caleb's Vulkan Engine", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, static_cast<int>(width), static_cast<int>(height), SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

	if (this->window == NULL)
	{
		printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
	}
}

void Application::CreateWindowSurface() 
{
	if (SDL_Vulkan_CreateSurface(window, this->m_instance, &this->m_windowSurface) != SDL_TRUE)
	{
		throw std::runtime_error("could not create window surface!");
	}
}

void Application::FindQueueFamilies() 
{

	uint32_t queueFamilyPropertyCount;
	VkQueueFamilyProperties* queueFamilies = nullptr;

	//no use for memory properties right now.
	VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;

	vkGetPhysicalDeviceMemoryProperties(this->m_physicalDevices[0], &physicalDeviceMemoryProperties);

	//similar maneuver to vkEnumeratePhysicalDevices
	vkGetPhysicalDeviceQueueFamilyProperties(this->m_physicalDevices[0], &queueFamilyPropertyCount, nullptr);

	if (queueFamilyPropertyCount == 0)
	{
		throw std::runtime_error("couldn't find any queue families...");
	}

	queueFamilies = new VkQueueFamilyProperties[queueFamilyPropertyCount];

	if (queueFamilies == nullptr) 
	{
		throw std::runtime_error("couldn't allocate queueFamilies array\n");
		return;
	}

	vkGetPhysicalDeviceQueueFamilyProperties(this->m_physicalDevices[device_index], &queueFamilyPropertyCount, queueFamilies);

	bool setGraphicsQueue = false;
	bool setPresentQueue = false;

	for (unsigned i = 0; i < queueFamilyPropertyCount; ++i)
	{
		if (((*(queueFamilies + i)).queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
		{
			graphicsFamily = i;
			setGraphicsQueue = true;

		}


		VkBool32 presentSupport = false;
		VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceSupportKHR(this->m_physicalDevices[device_index], i, this->m_windowSurface, &presentSupport));

		if (presentSupport)
		{
			presentFamily = i;
			setPresentQueue = true;
		}

		if (setGraphicsQueue && setPresentQueue)
		{
			break;
		}

	}

	delete[] queueFamilies;
}

void Application::CreateLogicalDevice() 
{
	VkDeviceQueueCreateInfo* deviceQueueCreateInfos = new VkDeviceQueueCreateInfo[2]; //presentation and graphics.

	uint32_t uniqueQueueFamilies[2] = { graphicsFamily, presentFamily };

	for (unsigned i = 0; i < 2; ++i)
	{
		VkDeviceQueueCreateInfo deviceQueueInfo = {}; //to be passed into deviceCreateInfo's struct members.
		deviceQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		deviceQueueInfo.flags = 0;
		deviceQueueInfo.pNext = nullptr;
		deviceQueueInfo.queueFamilyIndex = uniqueQueueFamilies[i];
		deviceQueueInfo.queueCount = 1;
		float queuePriority = 1.f;
		//THIS IS APPARENTLY REQUIRED --> REFERENCE BOOK DID NOT SHOW THIS...
		deviceQueueInfo.pQueuePriorities = &queuePriority; //normalized values between 0.f to 1.f that ranks the priority of the queue in the array.

		deviceQueueCreateInfos[i] = deviceQueueInfo;
	}

	//won't do many other optional features for now.
	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.geometryShader = VK_TRUE;
	deviceFeatures.tessellationShader = VK_TRUE;
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.flags = 0;
	deviceCreateInfo.pNext = nullptr;

	//layers are no right now.

	deviceCreateInfo.enabledLayerCount = 1;
	deviceCreateInfo.ppEnabledLayerNames = enabledLayerNames;

	deviceCreateInfo.enabledExtensionCount = 1;
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;

	deviceCreateInfo.pEnabledFeatures = &deviceFeatures; //call vkGetPhysicalDeviceFeatures to set additional features.

	deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfos;
	deviceCreateInfo.queueCreateInfoCount = 1;

	VK_CHECK_RESULT(vkCreateDevice(this->m_physicalDevices[device_index], &deviceCreateInfo, nullptr, &this->m_logicalDevice));

	delete[] deviceQueueCreateInfos;


}

void Application::CreateRenderPass() 
{
	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = this->depthFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentReference = 
	{
		1,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};


	VkAttachmentDescription colorAttachment =
	{
		//in memory tight scenarios, 
		// we can tell vulkan not to do anything 
		// that may make the data in this attachment inconsistent
		0,
		VK_FORMAT_B8G8R8A8_SRGB, //normalized format --> 0-1 unsigned float.
		VK_SAMPLE_COUNT_1_BIT, //samples -> no multisampling, so make it 1_bit.
		VK_ATTACHMENT_LOAD_OP_CLEAR, //load operation --> clear everything when the renderpass begins.
		VK_ATTACHMENT_STORE_OP_STORE, //store operation --> store resources to memory for later use.
		VK_ATTACHMENT_LOAD_OP_DONT_CARE, //stencil load operation
		VK_ATTACHMENT_STORE_OP_DONT_CARE, //stencil store operation

		//these two parameters can be expounded on with ***MULTIPASS RENDERING****.
		VK_IMAGE_LAYOUT_UNDEFINED, //really don't have an image to specify.
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, //final layout.
	};

	VkAttachmentReference colorAttachmentReference =
	{
		0,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription subpass =
	{
		0,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		0, //input attachment count
		nullptr, //pointer to input attachments
		1, //color attachment count
		&colorAttachmentReference,
		nullptr, //resolve attachments
		&depthAttachmentReference, //depth stencil attachment
		0, //preserve attachment count
		nullptr //pointer to reserved attachments.
	};

	VkAttachmentDescription attachments[2] = { colorAttachment, depthAttachment };

	VkRenderPassCreateInfo renderPassCreateInfo =
	{
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		nullptr, //pNext
		0, //flags are for future use...
		2, //attachment count
		attachments,
		1, //subpass count
		&subpass, //pointer to subpasses
		0, //dependency count
		nullptr //pointer to dependencies.
	};

	//two render passes are compatible if their attachment references are the same
	VK_CHECK_RESULT(vkCreateRenderPass(this->m_logicalDevice, &renderPassCreateInfo, nullptr, &this->m_renderPass));

}

void Application::CreateSwapChain() 
{

	VkSwapchainCreateInfoKHR swapChainInfo = {};
	swapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainInfo.surface = this->m_windowSurface;

	VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(this->m_physicalDevices[device_index], m_windowSurface, &deviceCapabilities));


	uint32_t surfaceFormatCount = 0;
	VkSurfaceFormatKHR* surfaceFormats = nullptr;

	VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(this->m_physicalDevices[device_index], this->m_windowSurface, &surfaceFormatCount, nullptr));

	//surfaceFormatCount now filled..
	if (surfaceFormatCount <= 0)
	{
		throw std::runtime_error("no surface formats available...");
	}

	surfaceFormats = new VkSurfaceFormatKHR[surfaceFormatCount];

	if (surfaceFormats == nullptr) 
	{
		throw std::runtime_error("failed to allocate surfaceFormats");
		return;
	}

	VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(this->m_physicalDevices[device_index], this->m_windowSurface, &surfaceFormatCount, surfaceFormats));


	//choose suitable format
	int surfaceIndex = -1;

	for (size_t i = 0; i < surfaceFormatCount; ++i)
	{
		if ((*(surfaceFormats + i)).format == VK_FORMAT_B8G8R8A8_SRGB && (*(surfaceFormats + i)).colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			surfaceIndex = i;
		}
	}

	if (surfaceIndex < 0) 
	{
		surfaceIndex = 0;
	}

	if (surfaceIndex < 0)
	{
		throw std::runtime_error("couldn't find a suitable format for swap chain");
	}


	this->imageCount = deviceCapabilities.minImageCount + 1;

	if (deviceCapabilities.maxImageCount > 0 && imageCount > deviceCapabilities.maxImageCount) 
	{
		this->imageCount = deviceCapabilities.maxImageCount;
	}

	swapChainInfo.minImageCount = this->imageCount;
	swapChainInfo.imageColorSpace = (*(surfaceFormats + surfaceIndex)).colorSpace;
	swapChainInfo.imageFormat = (*(surfaceFormats + surfaceIndex)).format;
	swapChainInfo.imageExtent = deviceCapabilities.currentExtent;
	swapChainInfo.imageArrayLayers = 1;
	swapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	
	if (graphicsFamily == presentFamily)
	{
		swapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; //present mode and graphics mode are the same.
		swapChainInfo.queueFamilyIndexCount = 0;
		swapChainInfo.pQueueFamilyIndices = nullptr;
	}
	else 
	{
		uint32_t queueFamilyIndices[] = { graphicsFamily, presentFamily };

		swapChainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapChainInfo.queueFamilyIndexCount = 2;
		swapChainInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	
	
	swapChainInfo.preTransform = deviceCapabilities.currentTransform;
	swapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR; //this is always guaranteed.
	swapChainInfo.clipped = VK_TRUE;
	swapChainInfo.oldSwapchain = nullptr; //resizing needs a reference to the old swap chain
	

	VK_CHECK_RESULT(vkCreateSwapchainKHR(this->m_logicalDevice, &swapChainInfo, nullptr, &this->swapChain));

	delete[] surfaceFormats;

}

void Application::CreateImageViews()
{

	this->swapChainImages = new VkImage[imageCount];

	VK_CHECK_RESULT(vkGetSwapchainImagesKHR(this->m_logicalDevice, swapChain, &this->imageCount, this->swapChainImages));

	//create imageview --> allow image to be seen in a different format.
	this->imageViews = new VkImageView[imageCount];

	for (unsigned i = 0; i < this->imageCount; ++i) {

		//this is nothing fancy, we won't be editing the color interpretation.
		VkComponentMapping componentMapping =
		{
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
		};

		//the view can only refer to one aspect of the parent image.
		VkImageSubresourceRange subresourceRange =
		{
			VK_IMAGE_ASPECT_COLOR_BIT,
			0, //base mip level
			1, //levelCount for mip levels
			0, //baseArrayLayer -> layer not an array image
			1, //layerCount for image array. 
		};

		VkImageViewCreateInfo imageViewCreateInfo =
		{
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			nullptr, //pNext
			0, //flags
			swapChainImages[i], //the created image above
			VK_IMAGE_VIEW_TYPE_2D, //view image type
			VK_FORMAT_B8G8R8A8_SRGB, //as long as the same bits per pixel, the parent and view will be compatible.
			componentMapping,
			subresourceRange
		};

		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;

		VK_CHECK_RESULT(vkCreateImageView(this->m_logicalDevice, &imageViewCreateInfo, nullptr, &imageViews[i]));

		

	}

}

VkPipelineShaderStageCreateInfo Application::CreateShaderModule(const char* name, VkShaderModule& shaderModule, VkShaderStageFlagBits stage) 
{
	std::ifstream file(name, std::ios::ate | std::ios::binary); //when we initialize, we std::ios::ate points to the end.

	if (!file.is_open())
	{
		throw std::runtime_error("failed to open shader file!");
	}

	char* buffer = nullptr;

	size_t fileSize = (size_t)file.tellg();

	buffer = new char[fileSize];

	file.seekg(0);

	file.read(buffer, fileSize);

	file.close();


	VkShaderModuleCreateInfo shaderVertModuleInfo =
	{
		VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		nullptr,
		0,
		fileSize,
		reinterpret_cast<const uint32_t*>(buffer)
	};

	VK_CHECK_RESULT(vkCreateShaderModule(this->m_logicalDevice, &shaderVertModuleInfo, nullptr, &shaderModule));

	VkPipelineShaderStageCreateInfo shaderStageInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		nullptr,
		0,
		stage,
		shaderModule,
		"main", //entry point -->pName
		nullptr //no specialization constants
	};

	delete[] buffer;

	return shaderStageInfo;

}

void Application::CreateFrameBuffers() 
{

	this->frameBuffer = new VkFramebuffer[imageCount];

	for (unsigned i = 0; i < imageCount; ++i) {

		VkImageView attachments[2] = {imageViews[i], this->depthImageView};

		//create framebuffer info
		VkFramebufferCreateInfo framebufferCreateInfo =
		{
			VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			nullptr, //pNext
			0, //reserved for future expansion.. flags are zero now.
			this->m_renderPass,
			2,// attachmentCount
			attachments, //attachments
			(uint32_t)width, //width
			(uint32_t)height, //height
			1 //1 layer
		};

		VK_CHECK_RESULT(vkCreateFramebuffer(this->m_logicalDevice, &framebufferCreateInfo, nullptr, &this->frameBuffer[i]));
	}

}

void Application::CreateBuffers() 
{
	
	//does nothing, because all the buffers I use are uniform buffers

}

void Application::CreateUniformBuffers()
{
	this->uniformBuffers.push_back(Buffer(sizeof(uTransformObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, (void*)&uTransform));
}

void Application::CreateImage
(
	uint32_t width, uint32_t height, uint32_t mipLevels,
	VkFormat format, VkImageTiling tiling,
	VkImageUsageFlags usage, VkMemoryPropertyFlags flags,
	VkImage& image, VkDeviceMemory& imageMemory, uint32_t arrayLayerCount
) 
{

	VkPhysicalDeviceMemoryProperties	vpdmp;
	vkGetPhysicalDeviceMemoryProperties(this->m_physicalDevices[device_index], &vpdmp);

	VkImageCreateInfo imageCreateInfo = { };
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.extent.width = width;
	imageCreateInfo.extent.height = height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = mipLevels;
	imageCreateInfo.arrayLayers = arrayLayerCount;
	imageCreateInfo.format = format;
	imageCreateInfo.tiling = tiling;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.usage = usage;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;


	VK_CHECK_RESULT(vkCreateImage(this->m_logicalDevice, &imageCreateInfo, nullptr, &image));

	VkMemoryRequirements memRequirements;

	vkGetImageMemoryRequirements(this->m_logicalDevice, image, &memRequirements);

	VkMemoryAllocateInfo memAllocInfo = {};
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocInfo.allocationSize = memRequirements.size;

	for (unsigned int i = 0; i < vpdmp.memoryTypeCount; i++)
	{
		VkMemoryType vmt = vpdmp.memoryTypes[i];
		VkMemoryPropertyFlags vmpf = vmt.propertyFlags;
		if ((memRequirements.memoryTypeBits & (1 << i)) != 0)
		{
			if ((vmpf & flags) != 0)
			{
				memAllocInfo.memoryTypeIndex = i;
				break;
			}
		}
	}

	VK_CHECK_RESULT(vkAllocateMemory(this->m_logicalDevice, &memAllocInfo, nullptr, &imageMemory));


	VK_CHECK_RESULT(vkBindImageMemory(this->m_logicalDevice, image, imageMemory, 0));

}

const VkQueue& Application::GraphicsQueue() 
{
	return this->graphicsQueue;
}

const VkCommandPool& Application::CommandPool() 
{
	return this->commandPool;
}

static VkCommandBuffer beginCmd() 
{
	assert(_Application != NULL);

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = _Application->CommandPool();
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer cmdBuffer;
	VK_CHECK_RESULT(vkAllocateCommandBuffers(_Application->LogicalDevice(), &allocInfo, &cmdBuffer));

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &beginInfo));

	return cmdBuffer;

}

static void endCmd(VkCommandBuffer commandBuffer) 
{
	assert(_Application != NULL);

	VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	VK_CHECK_RESULT(vkQueueSubmit(_Application->GraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE));
	VK_CHECK_RESULT(vkQueueWaitIdle(_Application->GraphicsQueue()));

	vkFreeCommandBuffers(_Application->LogicalDevice(), _Application->CommandPool(), 1, &commandBuffer);

}

static void TransitionImageLayout(VkImage image, VkFormat format,
								  VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) 
{
	VkCommandBuffer cmdBuffer = beginCmd();

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags srcStage = 0;
	VkPipelineStageFlags dstStage = 0;
	
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
		newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) 
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
			 newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else 
	{
		throw std::invalid_argument("bad layout transition");
	}

	//first two parameters 
	vkCmdPipelineBarrier(cmdBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	endCmd(cmdBuffer);

}

static void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) 
{
	VkCommandBuffer cmdBuffer = beginCmd();

	VkBufferImageCopy region = {};
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0,0,0 };
	region.imageExtent = 
	{
		width,
		height,
		1 
	};

	vkCmdCopyBufferToImage(cmdBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	endCmd(cmdBuffer);
}

VkFormat Application::findSupportedFormat(const std::vector<VkFormat>& possibleFormats, 
VkImageTiling tiling, VkFormatFeatureFlags features) 
{
	for (VkFormat format : possibleFormats) 
	{
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(this->m_physicalDevices[device_index], format, &properties);

		if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features) 
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features) 
		{
			return format;
		}

	}

	throw std::runtime_error("couldn't find a suitable format supported on the physical device.");
}
void Application::CreateDepthResources() 
{
	this->depthFormat = findSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, 
		VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

	//create depth image
	CreateImage((uint32_t)width, (uint32_t)height, 1, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
	VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, this->depthImage,
	this->depthImageMemory, 1);
	//create depth image view 

	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = this->depthImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = this->depthFormat;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VK_CHECK_RESULT(vkCreateImageView(this->m_logicalDevice, &viewInfo, nullptr, &this->depthImageView));

}

static std::string PathToTextures() 
{
	return "External/textures/";
}

static std::string PathToObjects() {

	return "External/objects/";
}


//TODO
void Application::CreateCubeMap() 
{
	/*unsigned char* textureData[6];
	int textureWidth, textureHeight, textureChannels;
	textureData[0] = stbi_load((PathToTextures() + std::string("texture.jpg")).c_str(), &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);
	textureData[1] = stbi_load((PathToTextures() + std::string("texture.jpg")).c_str(), &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);
	textureData[2] = stbi_load((PathToTextures() + std::string("texture.jpg")).c_str(), &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);
	textureData[3] = stbi_load((PathToTextures() + std::string("texture.jpg")).c_str(), &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);
	textureData[4] = stbi_load((PathToTextures() + std::string("texture.jpg")).c_str(), &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);
	textureData[5] = stbi_load((PathToTextures() + std::string("texture.jpg")).c_str(), &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);
	
	VkDeviceSize imageSize = textureWidth * textureHeight * 4 * 6;
	VkDeviceSize layerSize = imageSize / 6;

	Buffer stagingBuffer = Buffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, NULL);

	vkMapMemory(this->m_logicalDevice, stagingBuffer.memory, 0, VK_WHOLE_SIZE, 0, &stagingBuffer.mappedMemory);

	for (unsigned i = 0; i < 6; ++i) 
	{
		memcpy(reinterpret_cast<unsigned char*>(stagingBuffer.mappedMemory) + layerSize * i, textureData[i], layerSize);
	}

	vkUnmapMemory(this->m_logicalDevice, stagingBuffer.memory);*/

	/*ktxResult result;
	ktxTexture* ktxTexture;*/





}


void Application::GenerateMipMaps(VkImage image, VkFormat imgFormat, uint32_t textureWidth, uint32_t textureHeight, uint32_t mipLevels) 
{

	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(this->m_physicalDevices[device_index], imgFormat, &formatProperties);

	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) 
	{
		throw std::runtime_error("your physical device does not support linear blitting");
		//TODO: generate mipmap levels with software/storing mip levels in texture image and sampling that.
	}

	VkCommandBuffer cmdBuffer = beginCmd();

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	uint32_t mipWidth = textureWidth;
	uint32_t mipHeight = textureHeight;

	for (uint32_t i = 1; i < mipLevels; i++) 
	{
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, 
		nullptr, 0, nullptr, 1, &barrier);

		VkImageBlit blit = {};
		blit.srcOffsets[0] = { 0,0,0 };
		blit.srcOffsets[1] = { (int)(mipWidth), (int)(mipHeight), 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		
		blit.dstOffsets[0] = { 0,0,0 };
		blit.dstOffsets[1] = { (int)(mipWidth > 1 ? mipWidth / 2 : 1), (int)(mipHeight > 1 ? mipHeight / 2 : 1), 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(cmdBuffer, 
			image,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, 
			&blit, VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr, 0, nullptr, 1, &barrier);


		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;

	}

	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		0, nullptr, 0, nullptr, 1, &barrier);
	
	endCmd(cmdBuffer);

}


void Application::CreateTexture(const std::string& fileName)
{
	//TODO: ensure that the same texture isn't allocated twice.
	
	this->mTextures.push_back(Texture());
	Texture& newTexture = this->mTextures.back();

	int textureWidth, textureHeight, textureChannels;
	stbi_uc* pixels = stbi_load((PathToTextures() + fileName).c_str(), &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);
	
	VkDeviceSize imageSize = textureWidth * textureHeight * 4;

	if (pixels == NULL) 
	{
		throw std::runtime_error("failed to load texture image!");
	}

	uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(textureWidth, textureHeight)))) + 1;
	
	Buffer stagingBuffer = Buffer(static_cast<size_t>(imageSize), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, pixels);

	stbi_image_free(pixels);

	CreateImage(textureWidth, textureHeight, mipLevels, VK_FORMAT_R8G8B8A8_SRGB,
				VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, newTexture.mTextureImage, newTexture.mTextureMemory, 1);

	TransitionImageLayout(newTexture.mTextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
	
	copyBufferToImage(stagingBuffer.handle, newTexture.mTextureImage, (uint32_t)(textureWidth), (uint32_t)(textureHeight));
	
	GenerateMipMaps(newTexture.mTextureImage, VK_FORMAT_R8G8B8A8_SRGB, (uint32_t)textureWidth, (uint32_t)textureHeight, mipLevels);

	vkDestroyBuffer(this->m_logicalDevice, stagingBuffer.handle, nullptr);
	vkFreeMemory(this->m_logicalDevice, stagingBuffer.memory, nullptr);

	CreateTextureView(newTexture.mTextureImage, newTexture.mTextureImageView, mipLevels);

	CreateTextureSampler(newTexture.mTextureSampler, mipLevels);

	this->mTextures.back().mName = fileName;
}

void Application::CreateTextureView(const VkImage& textureImage, VkImageView& textureImageView, uint32_t mipLevels)
{

	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = textureImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;
	
	VK_CHECK_RESULT(vkCreateImageView(this->m_logicalDevice, &viewInfo, nullptr, &textureImageView));
}

void Application::CreateTextureSampler(VkSampler& textureSampler, uint32_t mipLevels)
{
	VkSamplerCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	createInfo.magFilter = VK_FILTER_LINEAR;
	createInfo.minFilter = VK_FILTER_LINEAR;
	createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	createInfo.anisotropyEnable = VK_TRUE;

	VkPhysicalDeviceProperties pdp = { };
	vkGetPhysicalDeviceProperties(this->m_physicalDevices[device_index], &pdp);

	createInfo.maxAnisotropy = pdp.limits.maxSamplerAnisotropy / 2.f;

	createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	createInfo.unnormalizedCoordinates = VK_FALSE;

	createInfo.compareEnable = VK_FALSE;
	createInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	createInfo.minLod = 0.f;
	createInfo.maxLod = static_cast<float>(mipLevels);
	createInfo.mipLodBias = 0.f; //optional...

	VK_CHECK_RESULT(vkCreateSampler(this->m_logicalDevice, &createInfo, nullptr, &textureSampler));
}


const VkPipeline& Application::GetTrianglePipeline()
{
	return this->pipeline;
}

const VkPipeline& Application::GetLinePipeline()
{
	return this->linePipeline;
}

VkPipelineLayout* Application::GetPipelineLayout() 
{
	return &(this->pipelineLayouts.back());

}

void Application::CreateDescriptorSets()
{
	//create descriptor pool
	VkDescriptorPoolSize poolSize[2] = {};
	poolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize[0].descriptorCount = 1; //max numbers of frames in flight.

	poolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSize[1].descriptorCount = 1 * 2; //max numbers of frames in flight times two to accomodate the gui.

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.poolSizeCount = (uint32_t)2;
	poolInfo.pPoolSizes = poolSize;
	poolInfo.maxSets = mTextures.size() * 2; //max numbers of frames in flight.

	VK_CHECK_RESULT(vkCreateDescriptorPool(this->m_logicalDevice, &poolInfo, nullptr, &this->descriptorPool));

	for (size_t i = 0; i < mTextures.size(); ++i) 
	{
		VkDescriptorSetAllocateInfo descriptorAllocInfo{};
		descriptorAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorAllocInfo.descriptorPool = this->descriptorPool;
		descriptorAllocInfo.descriptorSetCount = 1;
		descriptorAllocInfo.pSetLayouts = &descriptorSetLayout;

		VK_CHECK_RESULT(vkAllocateDescriptorSets(this->m_logicalDevice, &descriptorAllocInfo, &this->mTextures[i].mDescriptor));

	}


}


void Application::WriteDescriptorSets() 
{
	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = uniformBuffers[0].handle;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(uTransformObject);

	VkDescriptorImageInfo imageInfo = {};

	for (size_t i = 0; i < this->mTextures.size(); ++i) 
	{
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = this->mTextures[i].mTextureImageView;
		imageInfo.sampler = this->mTextures[i].mTextureSampler;

		VkWriteDescriptorSet descriptorWrite[2] = {};
		descriptorWrite[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite[0].dstSet = this->mTextures[i].mDescriptor;
		descriptorWrite[0].dstBinding = 0;
		descriptorWrite[0].dstArrayElement = 0;
		descriptorWrite[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite[0].descriptorCount = 1; //how many buffers
		descriptorWrite[0].pBufferInfo = &bufferInfo;
		descriptorWrite[0].pImageInfo = nullptr; // Optional
		descriptorWrite[0].pTexelBufferView = nullptr; // Optional

		descriptorWrite[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite[1].dstSet = this->mTextures[i].mDescriptor;
		descriptorWrite[1].dstBinding = 1;
		descriptorWrite[1].dstArrayElement = 0;
		descriptorWrite[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrite[1].descriptorCount = 1; //how many images
		descriptorWrite[1].pImageInfo = &imageInfo;
		descriptorWrite[1].pTexelBufferView = nullptr; // Optional

		vkUpdateDescriptorSets(this->m_logicalDevice, 2, descriptorWrite, 0, nullptr);
	}

}

void Application::CreatePipelineLayout() 
{
	VkPushConstantRange pushConstants[1];

	//this is for an object's model transformation.
	pushConstants[0].offset = 0;
	pushConstants[0].size = sizeof(glm::mat4);
	pushConstants[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkPipelineLayoutCreateInfo				pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.pNext = nullptr;
	pipelineLayoutCreateInfo.flags = 0;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = pushConstants;

	this->pipelineLayouts.push_back(VkPipelineLayout());

	VK_CHECK_RESULT(vkCreatePipelineLayout(this->m_logicalDevice, &pipelineLayoutCreateInfo, nullptr, &this->pipelineLayouts.back()));


}

void Application::CreatePipeline(VkPipelineShaderStageCreateInfo* pStages, int numStages, VkPrimitiveTopology primitiveTopology, VkPipeline& pipelineHandle)
{

	VkVertexInputBindingDescription vBindingDescription = {};
	vBindingDescription.stride = sizeof(struct Vertex);
	vBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	
	VkVertexInputAttributeDescription vInputAttribute[3] = {};

	//position
	vInputAttribute[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	vInputAttribute[0].location = 0;
	vInputAttribute[0].binding = 0;
	vInputAttribute[0].offset = offsetof(struct Vertex, pos);

	//normal
	vInputAttribute[1].format = vInputAttribute[0].format;
	vInputAttribute[1].location = 1;
	vInputAttribute[1].binding = 0;
	vInputAttribute[1].offset = offsetof(struct Vertex, nrm);

	//texture 
	vInputAttribute[2].format = VK_FORMAT_R32G32_SFLOAT; 
	vInputAttribute[2].location = 2;
	vInputAttribute[2].binding = 0;
	vInputAttribute[2].offset = offsetof(struct Vertex, uv);

	//all vertex info is in the shaders for now...
	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		nullptr, //pNext
		0, //flags
		1, //vertexBindingDescriptionCount
		&vBindingDescription,
		3, //attribute count
		vInputAttribute
	};

	VkPipelineInputAssemblyStateCreateInfo pipelineAssemblyCreateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		nullptr, //pNext
		0, //flags
		primitiveTopology,//topology
		VK_FALSE //primitiveRestartEnable
	};

	VkDynamicState dynamicState[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = 2;
	dynamicStateCreateInfo.pDynamicStates = dynamicState;

	
	VkPipelineViewportStateCreateInfo viewPortCreateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		nullptr, //pNext
		0, //flags
		1, //viewportCount
		nullptr, //pViewPorts
		1, //scissorCount
		nullptr, //pScissors
	};

	//TODO (RESEARCH): look at what the depth optinos and lineWidth options do.
	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		nullptr, //pNext
		0, //flags
		VK_FALSE, //depthClampEnable
		VK_FALSE, //rasterizerDiscardEnable
		VK_POLYGON_MODE_FILL, //polygonMode
		VK_CULL_MODE_BACK_BIT, //cullMode
		VK_FRONT_FACE_COUNTER_CLOCKWISE, //frontFace
		VK_FALSE, //depthBiasEnable
		0.f, //depthBiasConstantFactor
		0.f, //depthBiasClamp
		0.f, //depthBiasSlopeFactor
		1.f, //lineWidth
	};

	VkPipelineColorBlendAttachmentState			colorBlendAttachState;
	colorBlendAttachState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT
		| VK_COLOR_COMPONENT_G_BIT
		| VK_COLOR_COMPONENT_B_BIT
		| VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachState.blendEnable = VK_FALSE;
	colorBlendAttachState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
	colorBlendAttachState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
	colorBlendAttachState.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachState.alphaBlendOp = VK_BLEND_OP_ADD;



	VkPipelineColorBlendStateCreateInfo			colorBlendCreateInfo;
	colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendCreateInfo.pNext = nullptr;
	colorBlendCreateInfo.flags = 0;
	colorBlendCreateInfo.logicOpEnable = VK_FALSE;
	colorBlendCreateInfo.logicOp = VK_LOGIC_OP_COPY;

	colorBlendCreateInfo.attachmentCount = 1;
	colorBlendCreateInfo.pAttachments = &colorBlendAttachState;
	colorBlendCreateInfo.blendConstants[0] = 0;
	colorBlendCreateInfo.blendConstants[1] = 0;
	colorBlendCreateInfo.blendConstants[2] = 0;
	colorBlendCreateInfo.blendConstants[3] = 0;


	//depthstencil testing
	VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {};
	depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilCreateInfo.depthTestEnable = VK_TRUE;
	depthStencilCreateInfo.depthWriteEnable = VK_TRUE;
	depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	
	depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
	depthStencilCreateInfo.minDepthBounds = 0.f;
	depthStencilCreateInfo.maxDepthBounds = 1.f;
	
	//no stencil test for now.
	depthStencilCreateInfo.stencilTestEnable = VK_FALSE;
	depthStencilCreateInfo.front = {};
	depthStencilCreateInfo.back = {};

	
	VkPipelineMultisampleStateCreateInfo multiSampleCreateInfo = {};
	multiSampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multiSampleCreateInfo.sampleShadingEnable = VK_FALSE;
	multiSampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;


	VkGraphicsPipelineCreateInfo gfxPipelineCreateInfo =
	{
		VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		nullptr,
		0,
		(uint32_t)numStages,
		pStages,
		//TODO: VkPipelineVertexInputStateCreateInfo
		&vertexInputCreateInfo,
		//TODO: VkPipelineInputAssemblyStateCreateInfo,
		&pipelineAssemblyCreateInfo,
		//TODO: VkPipelineTessellationStateCreateInfo,
		nullptr,
		//TODO: VkPipelineViewportStateCreateInfo,
		&viewPortCreateInfo,
		//TODO: VkPipelineRasterizationStateCreateInfo,
		&rasterizationStateCreateInfo,
		//TODO: VkPipelineMultisampleStateCreateInfo,
		&multiSampleCreateInfo,
		//TODO: VkPipelineDepthStencilStateCreateInfo,
		&depthStencilCreateInfo,
		//TODO: VkPipelineColorBlendStateCreateInfo,
		&colorBlendCreateInfo,
		//VkPipelineDynamicStateCreateInfo,
		&dynamicStateCreateInfo,
		//VkPipelineLayout,
		this->pipelineLayouts.back(),
		//VkRenderPass,
		this->m_renderPass,
		//subpass,
		0,
		//basePipelineHandle,
		VK_NULL_HANDLE,
		//basePipelineIndex   
		0
	};

	VK_CHECK_RESULT(vkCreateGraphicsPipelines(this->m_logicalDevice, VK_NULL_HANDLE, 1, &gfxPipelineCreateInfo, nullptr, &pipelineHandle));

}

void Application::CreateCommandPools() 
{
	VkCommandPoolCreateInfo commandPoolCreateInfo = {};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; //recording commands every frame.
	commandPoolCreateInfo.queueFamilyIndex = 0; //only one physical device on initial development machine.

	VK_CHECK_RESULT(vkCreateCommandPool(this->m_logicalDevice, &commandPoolCreateInfo, nullptr, &this->commandPool));
}

void Application::CreateDescriptorSetLayout() 
{
	VkDescriptorSetLayoutBinding uTransformBinding{};
	uTransformBinding.binding = 0;
	uTransformBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uTransformBinding.descriptorCount = 1; //one uniform struct.
	uTransformBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; //we are going to use the transforms in the vertex shader.

	VkDescriptorSetLayoutBinding samplerBinding = {};
	samplerBinding.binding = 1;
	samplerBinding.descriptorCount = 1;
	samplerBinding.pImmutableSamplers = nullptr;
	samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; //we are going to use the sampler in the fragment shader.

	//VkDescriptorSetLayoutBinding 

	VkDescriptorSetLayoutBinding bindings[2] = { uTransformBinding, samplerBinding };

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 2;
	layoutInfo.pBindings = bindings;

	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(this->m_logicalDevice, &layoutInfo, nullptr, &descriptorSetLayout));
}

void Application::CreateCommandBuffers() 
{
	VkCommandBufferAllocateInfo cmdBufferCreateInfo = {};

	cmdBufferCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufferCreateInfo.commandPool = this->commandPool;
	cmdBufferCreateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; //cannot be called by other command buffers
	cmdBufferCreateInfo.commandBufferCount = 1;

	//TODO: cleanup
	VK_CHECK_RESULT(vkAllocateCommandBuffers(this->m_logicalDevice, &cmdBufferCreateInfo, &this->commandBuffer));
}

void Application::CreateSemaphores() 
{
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VK_CHECK_RESULT(vkCreateSemaphore(this->m_logicalDevice, &semaphoreInfo, nullptr, &this->imageAvailableSemaphore));

	VK_CHECK_RESULT(vkCreateSemaphore(this->m_logicalDevice, &semaphoreInfo, nullptr, &this->renderFinishedSemaphore));

}

void Application::CreateFences() 
{
	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; //to prevent indefinite waiting on first frame.

	VK_CHECK_RESULT(vkCreateFence(this->m_logicalDevice, &fenceInfo, nullptr, &this->inFlightFence));
}

void Application::RecreateSwapChain() 
{
	VK_CHECK_RESULT(vkDeviceWaitIdle(this->m_logicalDevice));

	VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(this->m_physicalDevices[device_index], m_windowSurface, &this->deviceCapabilities));

	for (unsigned i = 0; i < imageCount; ++i)
	{
		vkDestroyImageView(this->m_logicalDevice, this->imageViews[i], nullptr);
		vkDestroyFramebuffer(this->m_logicalDevice, this->frameBuffer[i], nullptr);
	}

	
	vkDestroyImageView(this->m_logicalDevice, this->depthImageView, nullptr);
	vkDestroyImage(this->m_logicalDevice, this->depthImage, nullptr);
	

	width = deviceCapabilities.currentExtent.width;
	height = deviceCapabilities.currentExtent.height;

	delete[] frameBuffer;
	delete[] swapChainImages;

	vkDestroySwapchainKHR(this->m_logicalDevice, this->swapChain, nullptr);

	CreateSwapChain();

	CreateImageViews();

	CreateDepthResources();

	CreateFrameBuffers();
}

void Application::ResizeViewport()
{
	int width, height;
	SDL_GetWindowSizeInPixels(this->window, &width, &height);
	this->m_viewPort.width = (float)width;
	this->m_viewPort.height = (float)height;
	this->m_scissor.extent.width = width;
	this->m_scissor.extent.height = height;
	uTransform.proj = glm::perspective(glm::radians(45.f), this->m_viewPort.width / this->m_viewPort.height, 0.1f, 1000.f); //proj
	uTransform.proj[1][1] *= -1.f;
	memcpy(uniformBuffers[0].mappedMemory, (void*)&uTransform, (size_t)(sizeof(uTransformObject)));

}

void Application::InitPhysicsWorld() 
{
	this->mPhysicsWorld = this->mPhysicsCommon.createPhysicsWorld();

	debugCube2.InitPhysics(ColliderType::CUBE);
	debugCube3.InitPhysics(ColliderType::CUBE, BodyType::STATIC);
	this->debugCube3.SetLinesArrayOffset(12); 

	this->mPhysicsWorld->setIsDebugRenderingEnabled(true);

	debugCube3.mPhysics.rigidBody->setIsDebugEnabled(true);
	debugCube2.mPhysics.rigidBody->setIsDebugEnabled(true);
	
	//the order they were added to the physics world
	reactphysics3d::DebugRenderer& debugRenderer = this->mPhysicsWorld->getDebugRenderer();
	debugRenderer.setIsDebugItemDisplayed(reactphysics3d::DebugRenderer::DebugItem::COLLIDER_AABB, true);
	
}
void Application::InitGui() 
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

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
	init_info.CheckVkResultFn = check_vk_result;
	ImGui_ImplVulkan_Init(&init_info);
}


int Application::GetTexture(const char* fileName) 
{
	for (size_t i = 0; i < mTextures.size(); ++i) 
	{
		if (strcmp(fileName, mTextures[i].mName.c_str()) == 0) 
		{
			return i;
		}
	}

	this->CreateTexture(std::string(fileName));
	return this->mTextures.size() - 1;
}

PhysicsWorld* Application::GetPhysicsWorld() 
{
	return this->mPhysicsWorld;
}

PhysicsCommon& Application::GetPhysicsCommon() 
{
	return this->mPhysicsCommon;
}

bool Application::init() 
{

	//uniform stuffs;
	uTransform.proj[1][1] *= -1.f;

	this->m_viewPort.width = (float)width;
	this->m_viewPort.height = (float)height;
	this->m_viewPort.minDepth = 0;
	this->m_viewPort.maxDepth = 1;

	this->m_scissor.extent.width = (uint32_t)width;
	this->m_scissor.extent.height = (uint32_t)height;
	
	CreateWindow();

	CreateInstance();

	CreateWindowSurface();

	//setup the debug callbacks... (optional...)

	EnumeratePhysicalDevices();
	
	//dunno what to do with this yet, but it fills out a large data structure that holds information about vendor-specific api info.
	
	//retrieve queue family properties 
	// --> group of queues that have identical capabilities and are able to run in parallel 
	//		--> could be arithmetic, passing shaders, stuff like that.
	FindQueueFamilies();

	CreateLogicalDevice();

	vkGetDeviceQueue(this->m_logicalDevice, graphicsFamily, 0, &graphicsQueue);
	vkGetDeviceQueue(this->m_logicalDevice, presentFamily, 0, &presentQueue);

	
	// If you want to draw a triangle:
	// - create renderpass object
	CreateSwapChain();

	CreateImageViews();
	

	VkPipelineShaderStageCreateInfo shaderVertStageInfo = CreateShaderModule("vert.spv", this->shaderVertModule, VK_SHADER_STAGE_VERTEX_BIT);
	VkPipelineShaderStageCreateInfo shaderFragModuleInfo = CreateShaderModule("frag.spv", this->shaderFragModule, VK_SHADER_STAGE_FRAGMENT_BIT);

	VkPipelineShaderStageCreateInfo shaderStages[] = { shaderVertStageInfo, shaderFragModuleInfo };

	////create layout 
	CreateCommandPools();
	CreateCommandBuffers();
	
	CreateUniformBuffers();
	
	CreateDepthResources();

	CreateDescriptorSetLayout();

	CreatePipelineLayout();

	this->debugCube = Object((PathToObjects() + "freddy.obj").c_str(), "texture.jpg", &this->pipelineLayouts.back());
	debugCube.mModelTransform = glm::mat4(5.f);
	debugCube.mModelTransform[3] = glm::vec4(1.f, 0, -20.f, 1);


	this->debugCube2 = Object((PathToObjects() + "gcube.obj").c_str(), "puppy1.bmp", &this->pipelineLayouts.back());
	this->debugCube2.mModelTransform[3] = glm::vec4(-10.f, 20, -5.f, 1);
	this->debugCube2.willDebugDraw(true);
	
	this->debugCube3 = Object((PathToObjects() + "base.obj").c_str(), "puppy1.bmp", &this->pipelineLayouts.back());
	const float dbScale = 30.f;
	this->debugCube3.mModelTransform = glm::mat4(dbScale);
	this->debugCube3.mModelTransform[3] = { 0.f, -5.f, 0.f, 1 };
	this->debugCube3.willDebugDraw(true);

	//this->isDebugEnabled = true;

	/*debugDrawObject.WillDraw(true);*/

	CreateDescriptorSets();
	WriteDescriptorSets();
	
	CreateRenderPass();
	CreateFrameBuffers();
	
	CreatePipeline(shaderStages, 2, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, this->pipeline);
	CreatePipeline(shaderStages, 2, VK_PRIMITIVE_TOPOLOGY_LINE_LIST, this->linePipeline);

	//commands
	CreateSemaphores();

	CreateFences();

	InitGui();

	InitPhysicsWorld();

	this->timeNow = SDL_GetPerformanceCounter();

	return true;


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
		return decimal(1.0);
	}
};

bool Application::UpdateInput()
{
	SDL_Event e;
	while (SDL_PollEvent(&e))
	{
		const Uint8* keystates = SDL_GetKeyboardState(nullptr);

		ImGui_ImplSDL2_ProcessEvent(&e);
		if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE))
		{
			return true;
		}

		if (e.type == SDL_KEYDOWN) 
		{
			glm::mat4 newTransform = uTransform.view;
			switch (e.key.keysym.sym)
			{
				case SDLK_w:
					newTransform = glm::mat4(X_BASIS, Y_BASIS, Z_BASIS, { 0,0, 30 * deltaTime, 1 }) * uTransform.view;
					break;
				case SDLK_s:
					newTransform = glm::mat4(X_BASIS, Y_BASIS, Z_BASIS, { 0,0, -30 * deltaTime, 1 }) * uTransform.view;
					break;
				case SDLK_a:
					newTransform = glm::mat4(X_BASIS, Y_BASIS, Z_BASIS, { 30 * deltaTime,0, 0, 1 }) * uTransform.view;
					break;
				case SDLK_d:
					newTransform = glm::mat4(X_BASIS, Y_BASIS, Z_BASIS, { -30 * deltaTime, 0, 0, 1 }) * uTransform.view;
					break;
			}

			uTransform.view = newTransform;

			memcpy(uniformBuffers[0].mappedMemory, (void*)&uTransform, (size_t)(sizeof(uTransformObject)));
		}
		else if (e.type == SDL_MOUSEMOTION) 
		{
			if ((keystates[SDL_SCANCODE_LSHIFT] && 
				e.button.button == SDL_BUTTON(SDL_BUTTON_LEFT) && 
				guiWindowIsFocused == false) || (e.button.button == SDL_BUTTON(SDL_BUTTON_MIDDLE)))
			{
				int deltaX = e.motion.xrel;
				int deltaY = e.motion.yrel;
				glm::mat4 newTransform = glm::mat4(X_BASIS, Y_BASIS, Z_BASIS, { deltaX * .1f, -deltaY * .1f, 0, 1 }) * uTransform.view;
				uTransform.view = newTransform;
				memcpy(uniformBuffers[0].mappedMemory, (void*)&uTransform, (size_t)(sizeof(uTransformObject)));
			}


			int mouseX = e.button.x;
			int mouseY = e.button.y;

			glm::vec4 cursorWindowPos(mouseX, mouseY, 1, 1);

			/*std::cout << "( " << cursorWindowPos.x << ", " << cursorWindowPos.y << " )\n";*/

			glm::vec4 cursorScreenPos = {};

			//ndc
			float cursorZ = 1.f;
			cursorScreenPos.x = (2 * cursorWindowPos.x) / width - 1;
			cursorScreenPos.y = (2 * cursorWindowPos.y) / height - 1; //vulkan is upside down.
			cursorScreenPos.z = 1;
			cursorScreenPos.w = cursorWindowPos.w;

			////eye

			////world 
			glm::vec4 ray_world = glm::inverse(uTransform.proj * uTransform.view) * cursorScreenPos;

			ray_world /= ray_world.w;

			//glm::vec3 ray_world = glm::vec3(cursorWorldPos);

			//2. cast ray from the mouse position and in the direction forward from the mouse position
			reactphysics3d::Vector3 rayStart(-uTransform.view[3].x, -uTransform.view[3].y, -uTransform.view[3].z);

			reactphysics3d::Vector3 rayEnd(ray_world.x, ray_world.y, ray_world.z);

			/*rayEnd.normalize();*/

			/*std::cout << "( " << rayEnd.x << ", " << rayEnd.y << ", " << rayEnd.z << " )\n";*/

			Ray ray(rayStart, rayEnd);

			RaycastInfo raycastInfo = {};

			RayCastObject callbackObject;

			this->mPhysicsWorld->raycast(ray, &callbackObject);

		}
		else 
		{
		}


		int deltaX = e.motion.xrel;
		int deltaY = e.motion.yrel;

		if (deltaX && deltaX != std::numeric_limits<int>::max())
		{
			if (e.type == SDL_MOUSEMOTION && e.button.button == SDL_BUTTON(SDL_BUTTON_RIGHT))
			{
				uTransform.view = glm::rotate(glm::mat4(1.f), (float)deltaX * .008f, glm::vec3(0, 1, 0)) * uTransform.view;
				memcpy(uniformBuffers[0].mappedMemory, (void*)&uTransform, (size_t)(sizeof(uTransformObject)));
			}
		}
		else if (deltaY && deltaY != std::numeric_limits<int>::max())
		{
			if (e.type == SDL_MOUSEMOTION && e.button.button == SDL_BUTTON(SDL_BUTTON_RIGHT))
			{
				uTransform.view = glm::rotate(glm::mat4(1.f), (float)deltaY * .008f, glm::vec3(1, 0, 0)) * uTransform.view;
				memcpy(uniformBuffers[0].mappedMemory, (void*)&uTransform, (size_t)(sizeof(uTransformObject)));
			}

		}
	}

	return false;
}

void Application::DrawGui() 
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
		guiWindowIsFocused = ImGui::IsWindowFocused();
		ImGui::End();
		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), this->commandBuffer);
		return;
	}

	guiWindowIsFocused = ImGui::IsWindowFocused();
	
	ImGui::End();
	ImGui::Render();
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), this->commandBuffer);


}

void Application::Render() 
{
	
	//vulkan shit
	VK_CHECK_RESULT(vkWaitForFences(this->m_logicalDevice, 1, &this->inFlightFence, VK_TRUE, UINT64_MAX));
	VK_CHECK_RESULT(vkResetFences(this->m_logicalDevice, 1, &this->inFlightFence));

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(this->m_logicalDevice, swapChain, UINT64_MAX, this->imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);


	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		RecreateSwapChain();
		ResizeViewport();
		return;
	}

	assert(result == VK_SUCCESS);

	VK_CHECK_RESULT(vkResetCommandBuffer(this->commandBuffer, 0));


	////always begin recording command buffers by calling vkBeginCommandBuffer --> just tells vulkan about the usage of a particular command buffer.
	//always begin recording command buffers by calling vkBeginCommandBuffer --> just tells vulkan about the usage of a particular command buffer.
	VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
	cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	//everything else is default...

	VK_CHECK_RESULT(vkBeginCommandBuffer(this->commandBuffer, &cmdBufferBeginInfo));

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = this->m_renderPass;
	renderPassInfo.framebuffer = frameBuffer[imageIndex];
	renderPassInfo.renderArea.offset = { 0,0 };
	renderPassInfo.renderArea.extent = deviceCapabilities.currentExtent;

	VkClearValue clearColors[2] = {};
	clearColors[0].color = { {0.f, 0.f, 0.f, 1.f} };
	clearColors[1].depthStencil = { 1.f, 0 };

	renderPassInfo.clearValueCount = 2;
	renderPassInfo.pClearValues = clearColors;

	//put this in a draw frame
	vkCmdBeginRenderPass(this->commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	//bind the graphics pipeline
	vkCmdBindPipeline(this->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline);
	/*vkCmdBindDescriptorSets(this->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipelineLayouts.back(), 0, 1, &descriptorSets, 0, nullptr);*/

	vkCmdSetViewport(this->commandBuffer, 0, 1, &this->m_viewPort);
	vkCmdSetScissor(this->commandBuffer, 0, 1, &this->m_scissor);

	debugCube.Draw(this->commandBuffer);
	debugCube2.Draw(this->commandBuffer);
	debugCube3.Draw(this->commandBuffer);

	DrawGui();

	vkCmdEndRenderPass(this->commandBuffer);

	VK_CHECK_RESULT(vkEndCommandBuffer(this->commandBuffer));

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { imageAvailableSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &this->commandBuffer;

	VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	VK_CHECK_RESULT(vkQueueSubmit(graphicsQueue, 1, &submitInfo, this->inFlightFence));

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &imageIndex;

	result = vkQueuePresentKHR(presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		RecreateSwapChain();
		ResizeViewport();
		return;
	}

	assert(result == VK_SUCCESS);

}

void Application::ComputeDeltaTime()
{
	this->timeBefore = this->timeNow;
	this->timeNow = SDL_GetPerformanceCounter();
	this->deltaTime = ((this->timeNow - this->timeBefore)) / (double)SDL_GetPerformanceFrequency();

}

void Application::UpdatePhysics(float& accumulator)
{
	const float timeStep = 1.f / 60;

	accumulator += this->deltaTime;

	while (accumulator >= timeStep)
	{
		this->mPhysicsWorld->update(timeStep);

		accumulator -= timeStep;
	}

	reactphysics3d::decimal factor = accumulator / timeStep;

	debugCube2.Update(factor);
	debugCube3.Update(factor);

}

void Application::loop()
{
	float accumulator = 0.f;

	//render graphics.
	bool quit = false;
	while (quit == false)
	{	
		ComputeDeltaTime();

		if (UpdateInput())
		{
			quit = true;
		}

		UpdatePhysics(accumulator);

		Render();
	}

	VK_CHECK_RESULT(vkDeviceWaitIdle(this->m_logicalDevice));



}

const std::vector<Texture>& Application::Textures() 
{
	return this->mTextures;

}


void Application::DestroyObjects() 
{
	debugCube.DestroyResources();
	debugCube2.DestroyResources();
	debugCube3.DestroyResources();
}

void Application::exit()
{

	CleanUpGui();

	DestroyObjects();

	vkDestroySemaphore(this->m_logicalDevice, this->imageAvailableSemaphore, nullptr);

	vkDestroySemaphore(this->m_logicalDevice, this->renderFinishedSemaphore, nullptr);

	vkFreeCommandBuffers(this->m_logicalDevice, this->commandPool, 1, &this->commandBuffer);
	
	vkDestroyCommandPool(this->m_logicalDevice, this->commandPool, nullptr);

	for (size_t i = 0; i < this->pipelineLayouts.size(); ++i) 
	{
		vkDestroyPipelineLayout(this->m_logicalDevice, this->pipelineLayouts[i], nullptr);
	}

	
	vkDestroyPipeline(this->m_logicalDevice, this->pipeline, nullptr);

	vkDestroyPipeline(this->m_logicalDevice, this->linePipeline, nullptr);

	vkDestroyDescriptorPool(this->m_logicalDevice, this->descriptorPool, nullptr);

	vkDestroyDescriptorSetLayout(this->m_logicalDevice, this->descriptorSetLayout, nullptr);

	for (unsigned i = 0; i < uniformBuffers.size(); ++i) 
	{
		vkDestroyBuffer(this->m_logicalDevice, uniformBuffers[i].handle, nullptr);
		vkFreeMemory(this->m_logicalDevice, uniformBuffers[i].memory, nullptr);
	}

	for (size_t i = 0; i < this->mTextures.size(); ++i) 
	{
		vkDestroySampler(this->m_logicalDevice, this->mTextures[i].mTextureSampler, nullptr);
		vkDestroyImageView(this->m_logicalDevice, this->mTextures[i].mTextureImageView, nullptr);
		vkDestroyImage(this->m_logicalDevice, this->mTextures[i].mTextureImage, nullptr);
		vkFreeMemory(this->m_logicalDevice, this->mTextures[i].mTextureMemory, nullptr);
	}


	vkDestroyImage(this->m_logicalDevice, this->depthImage, nullptr);
	vkDestroyImageView(this->m_logicalDevice, this->depthImageView, nullptr);
	vkFreeMemory(this->m_logicalDevice, this->depthImageMemory, nullptr);

	for (unsigned i = 0; i < imageCount; ++i)
	{
		vkDestroyImageView(this->m_logicalDevice, this->imageViews[i], nullptr);
		vkDestroyFramebuffer(this->m_logicalDevice, this->frameBuffer[i], nullptr);
	}

	vkDestroyShaderModule(this->m_logicalDevice, this->shaderVertModule, nullptr);

	vkDestroyShaderModule(this->m_logicalDevice, this->shaderFragModule, nullptr);
	
	vkDestroyRenderPass(this->m_logicalDevice, this->m_renderPass, nullptr);

	//this already destroys the images in it.
	vkDestroySwapchainKHR(this->m_logicalDevice, this->swapChain, nullptr);

	vkDestroyFence(this->m_logicalDevice, this->inFlightFence, nullptr);


	delete[] swapChainImages;
	delete[] m_physicalDevices;
	
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(this->m_instance, "vkDestroyDebugUtilsMessengerEXT");

	if (func != nullptr) 
	{
		func(this->m_instance, this->debugMessenger, nullptr);
	}


	this->mPhysicsCommon.destroyPhysicsWorld(this->mPhysicsWorld);

}


Application::~Application()
{
	vkDestroyDevice(this->m_logicalDevice, nullptr);

	vkDestroySurfaceKHR(this->m_instance, this->m_windowSurface, nullptr);

	vkDestroyInstance(this->m_instance, nullptr);

	SDL_DestroyWindow(window);

	SDL_Quit();
}


void Application::CleanUpGui() 
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
}

const VkPhysicalDevice& Application::PhysicalDevice() 
{
	return this->m_physicalDevices[device_index];
}

const VkDevice& Application::LogicalDevice()
{
	return this->m_logicalDevice;
}






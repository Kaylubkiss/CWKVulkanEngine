#include "Application.h"
#include <iostream>
#include <cassert>
#include <fstream>
#include <SDL2/SDL_vulkan.h>
#include <glm/gtc/matrix_transform.hpp>


#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480 


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
	glm::mat4(X_BASIS, Y_BASIS, Z_BASIS, W_BASIS), //model
	glm::lookAt(glm::vec3(0,0,10),glm::vec3(0,0,0), glm::vec3(0,-1,0)),
	glm::perspective(glm::radians(90.f), (float)SCREEN_WIDTH / SCREEN_HEIGHT, 0.1f, 10.f)
};




void Application::run() 
{
	//initialize all resources.
	if (!init()) 
	{
		std::cerr << "could not start the application!\n";
	}
	//render, update, render, update...
	loop();

	//cleanup resources
	exit();
}

VkResult Application::CreateInstance() 
{

	//create instance info.
	VkInstanceCreateInfo createInfo;

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

	if (SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr) != SDL_TRUE)
	{
		throw std::runtime_error("could not grab extensions from SDL!");
	}

	extensionNames = new const char* [extensionCount + 1];

	if (SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, &(extensionNames[0])) != SDL_TRUE)
	{
		throw std::runtime_error("could not grab extensions from SDL!");
	}
	extensionNames[extensionCount] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
	createInfo.enabledExtensionCount = extensionCount;
	createInfo.ppEnabledExtensionNames = &(extensionNames[0]);


	//this could be useful for logging, profiling, debugging, whatever.
	//it intercepts the API
	createInfo.ppEnabledLayerNames = enabledLayerNames;
	createInfo.enabledLayerCount = 1;

	//create instance.
	//this function, if successful, will create a "handle object"
	//and make pInstance the handle. A handle is always 64-bits wide.  

	//also, setting the pAllocator to null will make vulkan do its
	//own memory management, whereas we can create our own allocator
	//for vulkan to use
	return vkCreateInstance(&createInfo, nullptr, &this->m_instance);

}

void Application::EnumeratePhysicalDevices() 
{
	//list the physical devices
	uint32_t max_devices = 0;
	VkResult result;

	//vulkan will ignor whatever was set in physicalDeviceCount and overwrite max_devices 
	result = vkEnumeratePhysicalDevices(this->m_instance, &max_devices, nullptr);

	assert(result == VK_SUCCESS);

	if (max_devices == 0)
	{
		throw std::runtime_error("could not find any GPUs to use!\n");
	}

	this->m_physicalDevices = new VkPhysicalDevice[max_devices];

	result = vkEnumeratePhysicalDevices(this->m_instance, &max_devices, this->m_physicalDevices);

	assert(result == VK_SUCCESS);

	for (unsigned i = 0; i < max_devices; ++i)
	{
		VkPhysicalDeviceProperties properties;
		VkPhysicalDeviceFeatures features;

		vkGetPhysicalDeviceProperties(this->m_physicalDevices[i], &properties);
		vkGetPhysicalDeviceFeatures(this->m_physicalDevices[i], &features);

		if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && features.geometryShader)
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

	//TODO: cleanup
	this->window = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_VULKAN);

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

	vkGetPhysicalDeviceQueueFamilyProperties(this->m_physicalDevices[device_index], &queueFamilyPropertyCount, queueFamilies);

	bool setGraphicsQueue = false;
	bool setPresentQueue = false;

	for (unsigned i = 0; i < queueFamilyPropertyCount; ++i)
	{
		if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			graphicsFamily = i;
			setGraphicsQueue = true;

		}


		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(this->m_physicalDevices[device_index], i, this->m_windowSurface, &presentSupport);

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
	VkResult result;

	VkDeviceQueueCreateInfo* deviceQueueCreateInfos = new VkDeviceQueueCreateInfo[2]; //presentation and graphics.

	uint32_t uniqueQueueFamilies[2] = { graphicsFamily, presentFamily };
	//TODO: create logical device.
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

	result = vkCreateDevice(this->m_physicalDevices[device_index], &deviceCreateInfo, nullptr, &this->m_logicalDevice);

	assert(result == VK_SUCCESS);


}

void Application::CreateRenderPass() 
{
	VkResult result;

	VkAttachmentDescription renderPassAttachment =
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

	VkAttachmentReference attachmentReference =
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
		&attachmentReference,
		nullptr, //resolve attachments
		nullptr, //depth stencil attachments
		0, //preserve attachment count
		nullptr //pointer to reserved attachments.
	};


	VkRenderPassCreateInfo renderPassCreateInfo =
	{
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		nullptr, //pNext
		0, //flags are for future use...
		1, //attachment count
		&renderPassAttachment,
		1, //subpass count
		&subpass, //pointer to subpasses
		0, //dependency count
		nullptr //pointer to dependencies.
	};

	//two render passes are compatible if their attachment references are the same
	result = vkCreateRenderPass(this->m_logicalDevice, &renderPassCreateInfo, nullptr, &this->m_renderPass);

	assert(result == VK_SUCCESS);

}

void Application::CreateSwapChain() 
{
	VkResult result;

	VkSwapchainCreateInfoKHR swapChainInfo = {};
	swapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainInfo.surface = m_windowSurface;

	result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(this->m_physicalDevices[device_index], m_windowSurface, &deviceCapabilities);
	assert(result == VK_SUCCESS);

	uint32_t surfaceFormatCount = 0;
	VkSurfaceFormatKHR* surfaceFormats = nullptr;

	result = vkGetPhysicalDeviceSurfaceFormatsKHR(this->m_physicalDevices[device_index], this->m_windowSurface, &surfaceFormatCount, nullptr);
	assert(result == VK_SUCCESS);

	//surfaceFormatCount now filled..
	if (surfaceFormatCount == 0)
	{
		throw std::runtime_error("no surface formats available...");
	}

	surfaceFormats = new VkSurfaceFormatKHR[surfaceFormatCount];

	vkGetPhysicalDeviceSurfaceFormatsKHR(this->m_physicalDevices[device_index], this->m_windowSurface, &surfaceFormatCount, surfaceFormats);
	assert(result == VK_SUCCESS);

	//choose suitable format
	unsigned surfaceIndex = -1;

	for (unsigned i = 0; i < surfaceFormatCount; ++i)
	{
		if (surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			surfaceIndex = i;
		}
	}

	if (surfaceIndex < 0)
	{
		throw std::runtime_error("couldn't find a suitable format for swap chain");
	}

	swapChainInfo.minImageCount = deviceCapabilities.minImageCount;
	swapChainInfo.imageColorSpace = surfaceFormats[surfaceIndex].colorSpace;
	swapChainInfo.imageFormat = surfaceFormats[surfaceIndex].format;
	swapChainInfo.imageExtent = deviceCapabilities.currentExtent;
	swapChainInfo.imageArrayLayers = 1;
	swapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; //present mode and graphics mode are the same.
	swapChainInfo.queueFamilyIndexCount = 0;
	swapChainInfo.pQueueFamilyIndices = nullptr;
	swapChainInfo.preTransform = deviceCapabilities.currentTransform;
	swapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR; //this is always guaranteed.
	swapChainInfo.clipped = VK_TRUE;
	swapChainInfo.oldSwapchain = this->swapChain; //resizing needs a reference to the old swap chain.


	result = vkCreateSwapchainKHR(this->m_logicalDevice, &swapChainInfo, nullptr, &this->swapChain);
	assert(result == VK_SUCCESS);

	delete[] surfaceFormats;

}

void Application::CreateImageViews()
{
	VkResult result;

	result = vkGetSwapchainImagesKHR(this->m_logicalDevice, swapChain, &imageCount, nullptr);
	assert(result == VK_SUCCESS);

	this->swapChainImages = new VkImage[imageCount];
	result = vkGetSwapchainImagesKHR(this->m_logicalDevice, swapChain, &imageCount, this->swapChainImages);

	//create imageview --> allow image to be seen in a different format.
	this->imageViews = new VkImageView[imageCount];

	for (unsigned i = 0; i < imageCount; ++i) {

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

		//TODO: cleanup
		result = vkCreateImageView(this->m_logicalDevice, &imageViewCreateInfo, nullptr, &imageViews[i]);

		assert(result == VK_SUCCESS);


	}

}

VkPipelineShaderStageCreateInfo Application::CreateShaderModule(const char* name, VkShaderModule& shaderModule, VkShaderStageFlagBits stage) 
{
	VkResult result;
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

	//TODO: cleanup
	result = vkCreateShaderModule(this->m_logicalDevice, &shaderVertModuleInfo, nullptr, &shaderModule);

	assert(result == VK_SUCCESS);

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
	VkResult result;
	this->frameBuffer = new VkFramebuffer[imageCount];

	for (unsigned i = 0; i < imageCount; ++i) {

		//create framebuffer info
		VkFramebufferCreateInfo framebufferCreateInfo =
		{
			VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			nullptr, //pNext
			0, //reserved for future expansion.. flags are zero now.
			this->m_renderPass,
			1,
			&imageViews[i], //only captures the color of the image.
			SCREEN_WIDTH, //width
			SCREEN_HEIGHT, //height
			1 //1 layer
		};

		//TODO: cleanup
		result = vkCreateFramebuffer(this->m_logicalDevice, &framebufferCreateInfo, nullptr, &this->frameBuffer[i]);

		assert(result == VK_SUCCESS);
	}

}

void Application::CreateBuffers() 
{
	VkResult result;
	VkDeviceSize bufferSize;
	VkBufferCreateInfo bufferCreateInfo = {};

	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.pNext = nullptr;
	bufferCreateInfo.flags = 0;
	bufferCreateInfo.size = sizeof(uTransformObject);
	bufferSize = bufferCreateInfo.size;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

	bufferCreateInfo.queueFamilyIndexCount = 0;
	bufferCreateInfo.pQueueFamilyIndices = (const uint32_t*)nullptr;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;	// can only use CONCURRENT if .queueFamilyIndexCount > 0

	result = vkCreateBuffer(this->m_logicalDevice, &bufferCreateInfo, nullptr, &this->vkBuffer);
	assert(result == VK_SUCCESS);

	VkMemoryRequirements			memoryRequirments;
	vkGetBufferMemoryRequirements(this->m_logicalDevice, this->vkBuffer, &memoryRequirments);

	VkMemoryAllocateInfo			vmai;
	vmai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	vmai.pNext = nullptr;
	vmai.allocationSize = memoryRequirments.size;



	VkPhysicalDeviceMemoryProperties	vpdmp;
	vkGetPhysicalDeviceMemoryProperties(this->m_physicalDevices[device_index], &vpdmp);
	for (unsigned int i = 0; i < vpdmp.memoryTypeCount; i++)
	{
		VkMemoryType vmt = vpdmp.memoryTypes[i];
		VkMemoryPropertyFlags vmpf = vmt.propertyFlags;
		if ((memoryRequirments.memoryTypeBits & (1 << i)) != 0)
		{
			if ((vmpf & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0)
			{
				vmai.memoryTypeIndex = i;
				break;
			}
		}
	}


	VkDeviceMemory				vdm;
	result = vkAllocateMemory(this->m_logicalDevice, &vmai, nullptr, &vdm);
	assert(result == VK_SUCCESS);


	result = vkBindBufferMemory(this->m_logicalDevice, vkBuffer, vdm, 0);		// 0 is the offset
	assert(result == VK_SUCCESS);


	//fill data buffer --> THIS COULD BE ITS OWN MODULE...
	void* pGpuMemory;
	result = vkMapMemory(this->m_logicalDevice, vdm, 0, VK_WHOLE_SIZE, 0, &pGpuMemory);	// 0 and 0 are offset and flags
	memcpy(pGpuMemory, (void*)&uTransform, (size_t)bufferSize);
	vkUnmapMemory(this->m_logicalDevice, vdm);
	assert(result == VK_SUCCESS);


}

void Application::CreateDescriptorSets() 
{
	VkResult result;
	VkDescriptorSetLayoutBinding layoutBinding{};

	layoutBinding.binding = 0;
	layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layoutBinding.descriptorCount = 1; //one uniform struct.
	layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &layoutBinding;

	result = vkCreateDescriptorSetLayout(this->m_logicalDevice, &layoutInfo, nullptr, &descriptorSetLayout);
	assert(result == VK_SUCCESS);

	//create descriptor pool
	VkDescriptorPoolSize poolSize{};
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = 1; //just one uniform buffer.

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;
	poolInfo.maxSets = 1;

	result = vkCreateDescriptorPool(this->m_logicalDevice, &poolInfo, nullptr, &this->descriptorPool);
	assert(result == VK_SUCCESS);

	VkDescriptorSetAllocateInfo descriptorAllocInfo{};
	descriptorAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorAllocInfo.descriptorPool = this->descriptorPool;
	descriptorAllocInfo.descriptorSetCount = 1;
	descriptorAllocInfo.pSetLayouts = &descriptorSetLayout;

	result = vkAllocateDescriptorSets(this->m_logicalDevice, &descriptorAllocInfo, &descriptorSets);
	assert(result == VK_SUCCESS);


}

void Application::CreatePipeline(VkPipelineShaderStageCreateInfo* pStages, int numStages) 
{
	VkResult result;
	//all vertex info is in the shaders for now...
	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		nullptr, //pNext
		0, //flags
		0, //vertexBindingDescriptionCount
		nullptr,
		0,
		nullptr
	};

	VkPipelineInputAssemblyStateCreateInfo pipelineAssemblyCreateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		nullptr, //pNext
		0, //flags
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,//topology
		VK_FALSE //primitiveRestartEnable
	};

	VkViewport dummyViewPort =
	{
		0.f, 0.f, //viewport x and y
		SCREEN_WIDTH, SCREEN_HEIGHT, //width, height
		0.0f, 1.f //minDepth, maxDepth...
	};

	VkRect2D dummyScissor =
	{
		{ 0, 0 }, // offset
		{ SCREEN_WIDTH, SCREEN_HEIGHT } // extent
	};



	VkPipelineViewportStateCreateInfo viewPortCreateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		nullptr, //pNext
		0, //flags
		1, //viewportCount
		&dummyViewPort, //pViewPorts
		1, //scissorCount
		&dummyScissor, //pScissors
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
		VK_FRONT_FACE_CLOCKWISE, //frontFace
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

	VkPipelineLayoutCreateInfo				pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.pNext = nullptr;
	pipelineLayoutCreateInfo.flags = 0;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = (VkPushConstantRange*)nullptr;

	result = vkCreatePipelineLayout(this->m_logicalDevice, &pipelineLayoutCreateInfo, nullptr, &this->pipelineLayout);

	assert(result == VK_SUCCESS);

	VkPipelineMultisampleStateCreateInfo multiSampleCreateInfo = {};
	multiSampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multiSampleCreateInfo.sampleShadingEnable = VK_FALSE;
	multiSampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkGraphicsPipelineCreateInfo gfxPipelineCreateInfo =
	{
		VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		nullptr,
		0,
		numStages,
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
		nullptr,
		//TODO: VkPipelineColorBlendStateCreateInfo,
		&colorBlendCreateInfo,
		//TODO: VkPipelineDynamicStateCreateInfo,
		nullptr,
		//TODO: VkPipelineLayout,
		this->pipelineLayout,
		//TODO: VkRenderPass,
		this->m_renderPass,
		//TODO: subpass,
		0,
		//TODO: basePipelineHandle,
		VK_NULL_HANDLE,
		//TODO: basePipelineIndex   
		-1
	};

	//TODO: cleanup
	result = vkCreateGraphicsPipelines(this->m_logicalDevice, VK_NULL_HANDLE, 1, &gfxPipelineCreateInfo, nullptr, &this->pipeline);
	assert(result == VK_SUCCESS);

}

void Application::CreateCommandPools() 
{
	VkResult result;
	VkCommandPoolCreateInfo commandPoolCreateInfo = {};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; //recording commands every frame.
	commandPoolCreateInfo.queueFamilyIndex = 0; //only one physical device on initial development machine.

	//TODO: cleanup
	result = vkCreateCommandPool(this->m_logicalDevice, &commandPoolCreateInfo, nullptr, &this->commandPool);

	assert(result == VK_SUCCESS);

}

void Application::CreateCommandBuffers() 
{
	VkResult result;
	VkCommandBufferAllocateInfo cmdBufferCreateInfo = {};

	cmdBufferCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufferCreateInfo.commandPool = this->commandPool;
	cmdBufferCreateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; //cannot be called by other command buffers
	cmdBufferCreateInfo.commandBufferCount = 1;

	//TODO: cleanup
	result = vkAllocateCommandBuffers(this->m_logicalDevice, &cmdBufferCreateInfo, &this->commandBuffer);

	assert(result == VK_SUCCESS);

}

void CreateSemaphores() 
{


}

bool Application::init() 
{

	//uniform stuffs;


	uTransform.proj[1][1] *= -1;

	VkResult result = VK_SUCCESS;
	
	CreateWindow();

	result = CreateInstance();
	assert(result == VK_SUCCESS);

	CreateWindowSurface();

	//setup the debug callbacks... (optional...)

	EnumeratePhysicalDevices();
	
	//dunno what to do with this yet, but it fills out a large data structure that holds information about vendor-specific api info.
	
	//retrieve queue family properties 
	// --> group of queues that have identical capabilities and are able to run in parallel 
	//		--> could be arithmetic, passing shaders, stuff like that.
	FindQueueFamilies();

	CreateLogicalDevice();

	// If you want to draw a triangle:
	// - create renderpass object
	CreateRenderPass();

	CreateSwapChain();

	CreateImageViews();
	
	CreateFrameBuffers();

	VkPipelineShaderStageCreateInfo shaderVertStageInfo = CreateShaderModule("vert.spv", this->shaderVertModule, VK_SHADER_STAGE_VERTEX_BIT);
	VkPipelineShaderStageCreateInfo shaderFragModuleInfo = CreateShaderModule("frag.spv", this->shaderFragModule, VK_SHADER_STAGE_FRAGMENT_BIT);

	VkPipelineShaderStageCreateInfo shaderStages[] = { shaderVertStageInfo, shaderFragModuleInfo };

	////create layout 
	
	CreateBuffers();

	CreateDescriptorSets();

	CreatePipeline(shaderStages, 2);

	CreateCommandPools();

	CreateCommandBuffers();

	//create sync objects --> gpu work is asynchronous.
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; //to prevent indefinite waiting on first frame.

	//TODO: cleanup
	result = vkCreateSemaphore(this->m_logicalDevice, &semaphoreInfo, nullptr, &this->imageAvailableSemaphore);
	assert(result == VK_SUCCESS);
	
	//TODO: cleanup
	result = vkCreateSemaphore(this->m_logicalDevice, &semaphoreInfo, nullptr, &this->renderFinishedSemaphore);
	assert(result == VK_SUCCESS);

	//TODO: cleanup
	vkCreateFence(this->m_logicalDevice, &fenceInfo, nullptr, &this->inFlightFence);
	assert(result == VK_SUCCESS);

	vkGetDeviceQueue(this->m_logicalDevice, graphicsFamily, 0, &graphicsQueue);
	vkGetDeviceQueue(this->m_logicalDevice, presentFamily, 0, &presentQueue);

	//render graphics.
	SDL_Event e; bool quit = false;
	while (quit == false) 
	{
		while (SDL_PollEvent(&e)) 
		{
			//vulkan shit
			result = vkWaitForFences(this->m_logicalDevice, 1, &this->inFlightFence, VK_TRUE, UINT64_MAX);
			assert(result == VK_SUCCESS);
			result = vkResetFences(this->m_logicalDevice, 1, &this->inFlightFence);
			assert(result == VK_SUCCESS);

			uint32_t imageIndex;
			result = vkAcquireNextImageKHR(this->m_logicalDevice, swapChain, UINT64_MAX, this->imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
			assert(result == VK_SUCCESS);


			result = vkResetCommandBuffer(this->commandBuffer, 0);
			assert(result == VK_SUCCESS);


			////always begin recording command buffers by calling vkBeginCommandBuffer --> just tells vulkan about the usage of a particular command buffer.
			//always begin recording command buffers by calling vkBeginCommandBuffer --> just tells vulkan about the usage of a particular command buffer.
			VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
			cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			//everything else is default...

			result = vkBeginCommandBuffer(this->commandBuffer, &cmdBufferBeginInfo);

			assert(result == VK_SUCCESS);

			VkRenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = this->m_renderPass;
			renderPassInfo.framebuffer = frameBuffer[imageIndex];
			renderPassInfo.renderArea.offset = { 0,0 };
			renderPassInfo.renderArea.extent = deviceCapabilities.currentExtent;

			VkClearValue clearColor = { {{.0f, 0.5f, 0.5f, 1.0f}} };
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor;

			//put this in a draw frame
			vkCmdBeginRenderPass(this->commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			//bind the graphics pipeline
			vkCmdBindPipeline(this->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline);
			vkCmdBindDescriptorSets(this->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipelineLayout, 0, 1, &descriptorSets, 0, nullptr);

			vkCmdDraw(this->commandBuffer, 3, 1, 0, 0);


			vkCmdEndRenderPass(this->commandBuffer);

			result = vkEndCommandBuffer(this->commandBuffer);
			assert(result == VK_SUCCESS);

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

			result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, this->inFlightFence);
			assert(result == VK_SUCCESS);

			VkPresentInfoKHR presentInfo{};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pWaitSemaphores = signalSemaphores;

			VkSwapchainKHR swapChains[] = { swapChain };
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = swapChains;

			presentInfo.pImageIndices = &imageIndex;

			result = vkQueuePresentKHR(presentQueue, &presentInfo);
			assert(result == VK_SUCCESS);

			if (e.type == SDL_QUIT)
			{
				quit = true;
			}
		}
	}
	

	vkDeviceWaitIdle(this->m_logicalDevice);

	SDL_DestroyWindow(window);





	//not sure if I should do this yet





	return true;


}

void Application::loop() 
{
	//TODO


}

void Application::exit()
{
	//TODO: release all created resources from init.

	vkDeviceWaitIdle(this->m_logicalDevice);
	
	vkDestroyInstance(this->m_instance, nullptr);
	
	vkDestroyRenderPass(this->m_logicalDevice, this->m_renderPass, nullptr);

	vkDestroyDevice(this->m_logicalDevice, nullptr);

	delete[] m_physicalDevices;

	SDL_Quit();

}



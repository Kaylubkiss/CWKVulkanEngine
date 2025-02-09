#include "vkInit.h"
#include "vkUtility.h"
#include "vkDebug.h"
#include "vkResource.h"

#include <SDL2/SDL_vulkan.h>

namespace vk
{
	namespace init 
	{
		VkDebugUtilsMessengerCreateInfoEXT DebugMessengerCreateInfo()
		{
			VkDebugUtilsMessengerCreateInfoEXT createInfo = {};

			createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			createInfo.messageSeverity =
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			createInfo.messageType =
				VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

			createInfo.pfnUserCallback = vk::debug::debugCallback;

			return createInfo;
		}

		VkPipelineInputAssemblyStateCreateInfo AssemblyInputStateCreateInfo(VkPrimitiveTopology primitiveTopology)
		{
			VkPipelineInputAssemblyStateCreateInfo pipelineAssemblyCreateInfo =
			{
				VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
				nullptr, //pNext
				0, //flags
				primitiveTopology,//topology
				VK_FALSE //primitiveRestartEnable
			};


			return pipelineAssemblyCreateInfo;

		}

		VkPipelineVertexInputStateCreateInfo VertexInputStateCreateInfo()
		{

			//all vertex info is in the shaders for now...
			VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo =
			{
				VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
				nullptr, //pNext
				0, //flags
			};

			return vertexInputCreateInfo;

		}


		VkDescriptorSetLayout CreateDescriptorSetLayout(VkDevice logicalDevice)
		{
			VkDescriptorSetLayoutBinding uTransformBinding{};
			uTransformBinding.binding = 0;
			uTransformBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uTransformBinding.descriptorCount = 1; //one uniform struct.
			uTransformBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; //we are going to use the transforms in the vertex shader.

			VkDescriptorSetLayoutBinding samplerBinding = {};
			samplerBinding.binding = 2;
			samplerBinding.descriptorCount = 1;
			samplerBinding.pImmutableSamplers = nullptr;
			samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; //we are going to use the sampler in the fragment shader.

			VkDescriptorSetLayoutBinding uLightInfoBinding{};
			uLightInfoBinding.binding = 1;
			uLightInfoBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uLightInfoBinding.descriptorCount = 1;
			uLightInfoBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

			VkDescriptorSetLayoutBinding bindings[3] = { uTransformBinding, samplerBinding, uLightInfoBinding };


			VkDescriptorSetLayoutCreateInfo layoutInfo = {};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = 3;
			layoutInfo.pBindings = bindings;


			VkDescriptorSetLayout layout;

			VK_CHECK_RESULT(vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &layout));


			return layout;
		}

		std::array<VkVertexInputAttributeDescription, 3> VertexAttributeDescriptions()
		{
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

			return { vInputAttribute[0], vInputAttribute[1], vInputAttribute[2] };
		}


		VkInstance CreateInstance(SDL_Window* window)
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
			const char** extensionNames = nullptr;

			VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = vk::init::DebugMessengerCreateInfo();

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

			if (vk::util::CheckValidationSupport())
			{
				createInfo.ppEnabledLayerNames = vk::enabledLayerNames;
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

			VkInstance newInstance;

			VK_CHECK_RESULT(vkCreateInstance(&createInfo, nullptr, &newInstance))

			delete[] extensionNames;



		}

		DepthResources CreateDepthResources(const VkPhysicalDevice& p_device, const VkDevice& l_device, const VkViewport& viewport)
		{
			DepthResources nDepthResources = {};

			nDepthResources.depthFormat = vk::util::findSupportedFormat(p_device,
				{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
				VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

			//create depth image
			vk::rsc::CreateImage(p_device, l_device, (uint32_t)viewport.width, (uint32_t)viewport.height, 1, nDepthResources.depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, nDepthResources.depthImage,
				nDepthResources.depthImageMemory, 1);
			//create depth image view 

			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = nDepthResources.depthImage;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = nDepthResources.depthFormat;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			VK_CHECK_RESULT(vkCreateImageView(l_device, &viewInfo, nullptr, &nDepthResources.depthImageView));

			return { nDepthResources.depthImage, nDepthResources.depthImageMemory,
					 nDepthResources.depthImageView, nDepthResources.depthFormat };
		}
		
		VkRenderPass CreateRenderPass(const VkDevice l_device, const VkFormat& depthFormat)
		{
			VkAttachmentDescription depthAttachment = {};
			depthAttachment.format = depthFormat;
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
			VkRenderPass nRenderPass;
			VK_CHECK_RESULT(vkCreateRenderPass(l_device, &renderPassCreateInfo, nullptr, &nRenderPass));

			return nRenderPass;

		}


		VkSwapchainKHR CreateSwapChain(const VkPhysicalDevice p_device, uint32_t graphicsFamily, uint32_t presentFamily, const VkSurfaceKHR windowSurface)
		{

			VkSwapchainCreateInfoKHR swapChainInfo = {};
			swapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			swapChainInfo.surface = windowSurface;

			VkSurfaceCapabilitiesKHR deviceCapabilities;
			VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(p_device, windowSurface, &deviceCapabilities));


			uint32_t surfaceFormatCount = 0;
			VkSurfaceFormatKHR* surfaceFormats = nullptr;

			VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(p_device, windowSurface, &surfaceFormatCount, nullptr));

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

			VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(p_device, windowSurface, &surfaceFormatCount, surfaceFormats));


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
				uint32_t queueFamilyIndices[2] = { graphicsFamily, presentFamily };

				swapChainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
				swapChainInfo.queueFamilyIndexCount = 2;
				swapChainInfo.pQueueFamilyIndices = queueFamilyIndices;
			}


			swapChainInfo.preTransform = deviceCapabilities.currentTransform;
			swapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			swapChainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR; //this is always guaranteed.
			swapChainInfo.clipped = VK_TRUE;
			swapChainInfo.oldSwapchain = nullptr; //resizing needs a reference to the old swap chain

			VkSwapchainKHR nSwapChain;
			VK_CHECK_RESULT(vkCreateSwapchainKHR(l_device, &swapChainInfo, nullptr, &nSwapChain));

			delete[] surfaceFormats;

			return nSwapChain;

		}
	}
}
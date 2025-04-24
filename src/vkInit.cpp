#include "vkInit.h"
#include "vkUtility.h"
#include "vkDebug.h"
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

			createInfo.pfnUserCallback = vk::debug::debugMessengerCallback;

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


		VkDescriptorSetLayout DescriptorSetLayout(VkDevice logicalDevice)
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

			VkDescriptorSetLayoutBinding bindings[2] = { uTransformBinding, samplerBinding };

			VkDescriptorSetLayoutCreateInfo layoutInfo = {};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = 2;
			layoutInfo.pBindings = bindings;


			VkDescriptorSetLayout layout;
			VK_CHECK_RESULT(vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &layout));


			return layout;
		}

		std::array<VkVertexInputAttributeDescription, 3> VertexAttributeDescriptions()
		{

			//TODO: check VkPhysicalDeviceLimits!!! 
			// binding -> maxVertexInputBindings, 
			// location -> maxVertexInputAttributes
			// offset -> maxVertexInputAttributeOffset
			//
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
			appInfo.pNext = nullptr;

			createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			createInfo.flags = 0;

			//linked list of structures to pass to the create instance func.
			//--> look into it later.
			createInfo.pNext = nullptr;


			//we won't be doing any extension for now --> look into it at a later time.
			//need to get sdl extensionss
			unsigned int sdl_extensionCount = 0;

			VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = vk::init::DebugMessengerCreateInfo();

			if (SDL_Vulkan_GetInstanceExtensions(window, &sdl_extensionCount, nullptr) != SDL_TRUE)
			{
				throw std::runtime_error("could not grab extensions from SDL!");
			}

			std::vector<const char*> extensionNames(sdl_extensionCount);

			if (SDL_Vulkan_GetInstanceExtensions(window, &sdl_extensionCount, extensionNames.data()) != SDL_TRUE)
			{
				throw std::runtime_error("could not grab extensions from SDL!");
			}


			//find other instance extensions.
			uint32_t extensionPropertyCount = 0;
			vkEnumerateInstanceExtensionProperties(nullptr, &extensionPropertyCount, nullptr);

			std::vector<VkExtensionProperties> extensionProperties(extensionPropertyCount);

			vkEnumerateInstanceExtensionProperties(nullptr, &extensionPropertyCount, extensionProperties.data());

			for (auto& property : extensionProperties)
			{
				if (strcmp(property.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
					extensionNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
					break;
				}
			}


			//this could be useful for logging, profiling, debugging, whatever.
			//it intercepts the API
			if (vk::util::CheckValidationSupport())
			{
				createInfo.ppEnabledLayerNames = vk::instanceLayerExtensions;
				createInfo.enabledLayerCount = 1;
			}

			createInfo.enabledExtensionCount = extensionNames.size();
			createInfo.ppEnabledExtensionNames = extensionNames.data();

			createInfo.pNext = &debugCreateInfo;
			createInfo.pApplicationInfo = &appInfo;

			//create instance.
			//this function, if successful, will create a "handle object"
			//and make pInstance the handle. A handle is always 64-bits wide.  

			//also, setting the pAllocator to null will make vulkan do its
			//own memory management, whereas we can create our own allocator
			//for vulkan to use

			VkInstance newInstance;

			VK_CHECK_RESULT(vkCreateInstance(&createInfo, nullptr, &newInstance))

			return newInstance;

		}

		

		inline VkSampler CreateTextureSampler(const VkPhysicalDevice p_device, const VkDevice l_device, uint32_t mipLevels)
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
			vkGetPhysicalDeviceProperties(p_device, &pdp);

			createInfo.maxAnisotropy = pdp.limits.maxSamplerAnisotropy / 2.f;

			createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
			createInfo.unnormalizedCoordinates = VK_FALSE;

			createInfo.compareEnable = VK_FALSE;
			createInfo.compareOp = VK_COMPARE_OP_ALWAYS;

			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.minLod = 0.f;
			createInfo.maxLod = static_cast<float>(mipLevels);
			createInfo.mipLodBias = 0.f; //optional...

			VkSampler nTextureSampler;
			VK_CHECK_RESULT(vkCreateSampler(l_device, &createInfo, nullptr, &nTextureSampler));

			return nTextureSampler;
		}

		VkCommandPool CommandPool(const VkDevice& l_device, VkCommandPoolCreateFlags createFlag) 
		{

			VkCommandPoolCreateInfo commandPoolCreateInfo = {};
			commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			commandPoolCreateInfo.flags = createFlag; //recording commands every frame.
			commandPoolCreateInfo.queueFamilyIndex = 0; //only one physical device on initial development machine.

			VkCommandPool cmdPool;
			VK_CHECK_RESULT(vkCreateCommandPool(l_device, &commandPoolCreateInfo, nullptr, &cmdPool));

			return cmdPool;
		}

		VkCommandBuffer CommandBuffer(const VkDevice l_device, const VkCommandPool cmdPool, VkCommandBufferLevel cmdLevel)
		{
			VkCommandBufferAllocateInfo cmdBufferCreateInfo = {};

			cmdBufferCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			cmdBufferCreateInfo.commandPool = cmdPool;
			cmdBufferCreateInfo.level = cmdLevel; //cannot be called by other command buffers
			cmdBufferCreateInfo.commandBufferCount = 1;

			VkCommandBuffer nCommandBuffer;
			VK_CHECK_RESULT(vkAllocateCommandBuffers(l_device, &cmdBufferCreateInfo, &nCommandBuffer));

			return nCommandBuffer;
		}

		VkSemaphore CreateSemaphore(const VkDevice l_device)
		{
			VkSemaphoreCreateInfo semaphoreInfo = {};
			semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

			VkSemaphore nSemaphore;
			VK_CHECK_RESULT(vkCreateSemaphore(l_device, &semaphoreInfo, nullptr, &nSemaphore))
			
			return nSemaphore;
		}

		VkFence CreateFence(const VkDevice l_device) 
		{
			VkFenceCreateInfo fenceInfo = {};
			fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; //to prevent indefinite waiting on first frame.

			VkFence nFence;
			VK_CHECK_RESULT(vkCreateFence(l_device, &fenceInfo, nullptr, &nFence));

			return nFence;
		}

		VkShaderModule ShaderModule(const VkDevice& l_device, const char* filename)
		{
			
			std::string source_file = vk::util::ReadFile(filename);

			VkShaderModuleCreateInfo shaderVertModuleInfo =
			{
				VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
				nullptr,
				0,
				source_file.size(),
				reinterpret_cast<const uint32_t*>(source_file.data())
			};

			

			VkShaderModule nShaderModule;
			VK_CHECK_RESULT(vkCreateShaderModule(l_device, &shaderVertModuleInfo, nullptr, &nShaderModule));

			return nShaderModule;
		}

		VkPipelineShaderStageCreateInfo PipelineShaderStageCreateInfo(const VkShaderModule& shaderModule, VkShaderStageFlagBits stage) 
		{
			VkPipelineShaderStageCreateInfo nShaderStageInfo =
			{
				VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				nullptr,
				0,
				stage,
				shaderModule,
				"main", //entry point -->pName
				nullptr //no specialization constants
			};

			return nShaderStageInfo;
		}

		VkPipelineLayout CreatePipelineLayout(const VkDevice l_device, const VkDescriptorSetLayout descriptorSetLayout)
		{
			//TODO: check if the amount of set layouts exceed the physical limit!!!

			//this is for an object's model transformation.
			VkPushConstantRange pushConstants[1];
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

			VkPipelineLayout nPipelineLayout;
			VK_CHECK_RESULT(vkCreatePipelineLayout(l_device, &pipelineLayoutCreateInfo, nullptr, &nPipelineLayout));

			return nPipelineLayout;


		}


		VkDescriptorPool DescriptorPool(const VkDevice l_device) 
		{
			const uint32_t poolSizeCount = 2;
			VkDescriptorPoolSize poolSize[poolSizeCount] = {};
			poolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			poolSize[0].descriptorCount = 2; //max numbers of frames in flight.

			//we are concerned about the fragment stage, so we double the descriptor count here.
			poolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			poolSize[1].descriptorCount = 1 * 2; //max numbers of frames in flight times two to accomodate the gui.

			VkDescriptorPoolCreateInfo poolInfo{};
			poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			poolInfo.poolSizeCount = poolSizeCount;
			poolInfo.pPoolSizes = poolSize;
			poolInfo.maxSets = 1000; //how many descriptor sets in this pool?

			VkDescriptorPool nDescriptorPool;
			VK_CHECK_RESULT(vkCreateDescriptorPool(l_device, &poolInfo, nullptr, &nDescriptorPool));

			return nDescriptorPool;
		}


		VkDescriptorSet DescriptorSet(const VkDevice l_device, const VkDescriptorPool dscPool, const VkDescriptorSetLayout dscLayout)
		{
			VkDescriptorSetAllocateInfo descriptorAllocInfo{};
			descriptorAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			descriptorAllocInfo.descriptorPool = dscPool;
			descriptorAllocInfo.descriptorSetCount = 1;
			descriptorAllocInfo.pSetLayouts = &dscLayout;

			VkDescriptorSet nDescriptorSet;
			VK_CHECK_RESULT(vkAllocateDescriptorSets(l_device, &descriptorAllocInfo, &nDescriptorSet));

			return nDescriptorSet;
		}

		VkPipeline CreatePipeline(const VkDevice l_device, const VkPipelineLayout pipelineLayout, const VkRenderPass renderPass, VkPipelineShaderStageCreateInfo* pStages, int numStages, VkPrimitiveTopology primitiveTopology)
		{
			auto vAttribs = vk::init::VertexAttributeDescriptions();

			VkVertexInputBindingDescription vBindingDescription = {};
			vBindingDescription.stride = sizeof(struct Vertex);
			vBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = vk::init::VertexInputStateCreateInfo();
			vertexInputCreateInfo.vertexBindingDescriptionCount = 1; //vertexBindingDescriptionCount
			vertexInputCreateInfo.pVertexBindingDescriptions = &vBindingDescription,
				vertexInputCreateInfo.vertexAttributeDescriptionCount = vAttribs.size(); //attribute count
			vertexInputCreateInfo.pVertexAttributeDescriptions = vAttribs.data();

			VkPipelineInputAssemblyStateCreateInfo pipelineAssemblyCreateInfo = vk::init::AssemblyInputStateCreateInfo(primitiveTopology);

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
				// VkPipelineVertexInputStateCreateInfo
				&vertexInputCreateInfo,
				// VkPipelineInputAssemblyStateCreateInfo,
				&pipelineAssemblyCreateInfo,
				// VkPipelineTessellationStateCreateInfo,
				nullptr,
				// VkPipelineViewportStateCreateInfo,
				&viewPortCreateInfo,
				// VkPipelineRasterizationStateCreateInfo,
				&rasterizationStateCreateInfo,
				// VkPipelineMultisampleStateCreateInfo,
				&multiSampleCreateInfo,
				// VkPipelineDepthStencilStateCreateInfo,
				&depthStencilCreateInfo,
				// VkPipelineColorBlendStateCreateInfo,
				&colorBlendCreateInfo,
				// VkPipelineDynamicStateCreateInfo,
				&dynamicStateCreateInfo,
				// VkPipelineLayout,
				pipelineLayout,
				// VkRenderPass,
				renderPass,
				// subpass,
				0,
				// basePipelineHandle,
				VK_NULL_HANDLE,
				// basePipelineIndex   
				0
			};


			VkPipeline pipelineHandle;
			VK_CHECK_RESULT(vkCreateGraphicsPipelines(l_device, VK_NULL_HANDLE, 1, &gfxPipelineCreateInfo, nullptr, &pipelineHandle));

			return pipelineHandle;
		}

		VkRenderPass RenderPass(const VkDevice l_device, const VkFormat& depthFormat)
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




	}
}
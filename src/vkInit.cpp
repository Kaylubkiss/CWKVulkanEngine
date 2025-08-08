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


		VkVertexInputBindingDescription VertexInputBindingDescription(uint32_t binding) 
		{
			return { binding, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX };
		}

		VkPipelineVertexInputStateCreateInfo PipelineVertexInputStateCreateInfo()
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


		VkDescriptorSetLayout DescriptorSetLayout(VkDevice logicalDevice, VkDescriptorSetLayoutBinding* bindings, uint32_t sizeOfBindings)
		{

			VkDescriptorSetLayoutCreateInfo layoutInfo = {};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = sizeOfBindings;
			layoutInfo.pBindings = bindings;

			VkDescriptorSetLayout layout;
			VK_CHECK_RESULT(vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &layout));

			return layout;
		}

		//buffer descriptor
		VkWriteDescriptorSet WriteDescriptorSet(VkDescriptorSet dstSet, VkDescriptorType type, uint32_t binding, VkDescriptorBufferInfo* bufferInfo, uint32_t descriptorCount) 
		{
			VkWriteDescriptorSet nDescriptorSet = {};
			nDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			nDescriptorSet.dstSet = dstSet;
			nDescriptorSet.descriptorType = type;
			nDescriptorSet.dstBinding = binding;
			nDescriptorSet.pBufferInfo = bufferInfo;
			nDescriptorSet.descriptorCount = descriptorCount;
			return nDescriptorSet;
		}

		//image descriptor
		VkWriteDescriptorSet WriteDescriptorSet(VkDescriptorSet dstSet, VkDescriptorType type, uint32_t binding, VkDescriptorImageInfo* imageInfo, uint32_t descriptorCount) 
		{
			VkWriteDescriptorSet nDescriptorSet = {};
			nDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			nDescriptorSet.dstSet = dstSet;
			nDescriptorSet.descriptorType = type;
			nDescriptorSet.dstBinding = binding;
			nDescriptorSet.pImageInfo = imageInfo;
			nDescriptorSet.descriptorCount = descriptorCount;
			return nDescriptorSet;
		}

		VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo(VkDescriptorPool descriptorPool, const VkDescriptorSetLayout* descriptorLayout, uint32_t descriptorSetCount) 
		{
			VkDescriptorSetAllocateInfo nInfo = {};
			nInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			nInfo.descriptorPool = descriptorPool;
			nInfo.pSetLayouts = descriptorLayout;
			nInfo.descriptorSetCount = descriptorSetCount;
			return nInfo;
		}


		VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo(const std::vector<VkDescriptorSetLayoutBinding>& bindings)
		{
			VkDescriptorSetLayoutCreateInfo nInfo = {};
			nInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			nInfo.pBindings = bindings.data();
			nInfo.bindingCount = (uint32_t)bindings.size();
			return nInfo;
		}


		VkRenderPassCreateInfo RenderPassCreateInfo() 
		{
			VkRenderPassCreateInfo nInfo = {};
			nInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			return nInfo;
		}

		VkFramebufferCreateInfo FramebufferCreateInfo() 
		{
			VkFramebufferCreateInfo nInfo = {};
			nInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			return nInfo;
		}

		VkImageCreateInfo ImageCreateInfo() 
		{
			VkImageCreateInfo nCreateInfo = {};
			nCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			return nCreateInfo;
		}

		VkImageViewCreateInfo ImageViewCreateInfo() 
		{
			VkImageViewCreateInfo nInfo = {};
			nInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			return nInfo;
		}
		

		VkSamplerCreateInfo SamplerCreateInfo() 
		{
			VkSamplerCreateInfo nInfo = {};
			nInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			return nInfo;
		}

		VkMemoryAllocateInfo MemoryAllocateInfo() 
		{
			VkMemoryAllocateInfo memAllocInfo{};
			memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			return memAllocInfo;
		}

		VkMappedMemoryRange MappedMemoryRange() {

			VkMappedMemoryRange mappedMemInfo = {};
			mappedMemInfo.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			return mappedMemInfo;
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

		VkPipelineLayout CreatePipelineLayout(const VkDevice l_device, const VkDescriptorSetLayout descriptorSetLayout, std::vector<VkPushConstantRange>& pushConstantRanges)
		{
			//TODO: check if the amount of set layouts exceed the physical limit!!!
			VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
			pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutCreateInfo.pNext = nullptr;
			pipelineLayoutCreateInfo.flags = 0;
			pipelineLayoutCreateInfo.setLayoutCount = 1;
			pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
			pipelineLayoutCreateInfo.pushConstantRangeCount = pushConstantRanges.size();
			pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges.data();

			VkPipelineLayout nPipelineLayout;
			VK_CHECK_RESULT(vkCreatePipelineLayout(l_device, &pipelineLayoutCreateInfo, nullptr, &nPipelineLayout));

			return nPipelineLayout;


		}

		VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo() 
		{
			VkPipelineLayoutCreateInfo nInfo = {};
			nInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			return nInfo;
		}

		VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo(const std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t maxSets) 
		{
			VkDescriptorPoolCreateInfo poolInfo = {};

			poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			poolInfo.poolSizeCount = (uint32_t)(poolSizes.size());
			poolInfo.pPoolSizes = poolSizes.data();
			poolInfo.maxSets = maxSets;

			return poolInfo;
		}

		VkDescriptorPoolSize DescriptorPoolSize(VkDescriptorType type, uint32_t dscCount) 
		{
			VkDescriptorPoolSize nPoolSize = {};

			nPoolSize.type = type;
			nPoolSize.descriptorCount = dscCount;

			return nPoolSize;
		}

		VkDescriptorPool DescriptorPool(const VkDevice l_device, const VkDescriptorPoolCreateInfo& poolInfo) 
		{
			VkDescriptorPool nDescriptorPool;
			VK_CHECK_RESULT(vkCreateDescriptorPool(l_device, &poolInfo, nullptr, &nDescriptorPool));

			return nDescriptorPool;
		}


		VkDescriptorSet DescriptorSet(const VkDevice l_device, const VkDescriptorPool dscPool, const VkDescriptorSetLayout dscLayout, uint32_t dscCount)
		{
			VkDescriptorSetAllocateInfo descriptorAllocInfo{};
			descriptorAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			descriptorAllocInfo.descriptorPool = dscPool;
			descriptorAllocInfo.descriptorSetCount = dscCount;
			descriptorAllocInfo.pSetLayouts = &dscLayout;

			VkDescriptorSet nDescriptorSet;
			VK_CHECK_RESULT(vkAllocateDescriptorSets(l_device, &descriptorAllocInfo, &nDescriptorSet));

			return nDescriptorSet;
		}

		VkDescriptorSetLayoutBinding DescriptorLayoutBinding(uint32_t binding, uint32_t descriptorCount, VkDescriptorType descriptorType, VkShaderStageFlags shaderStage)
		{
			VkDescriptorSetLayoutBinding dscSetLayoutBinding = {};

			dscSetLayoutBinding.binding = binding;
			dscSetLayoutBinding.descriptorCount = descriptorCount;
			dscSetLayoutBinding.descriptorType = descriptorType;
			dscSetLayoutBinding.stageFlags = shaderStage;

			return dscSetLayoutBinding;
		}

		VkPushConstantRange PushConstantRange(uint32_t offset, uint32_t size, VkShaderStageFlags shaderStages)
		{
			VkPushConstantRange nPushConstant = {};
			nPushConstant.offset = offset;
			nPushConstant.size = size;
			nPushConstant.stageFlags = shaderStages;

			return nPushConstant;
		}


		VkPipelineInputAssemblyStateCreateInfo PipelineInputAssemblyStateCreateInfo(
			VkPrimitiveTopology topology, 
			VkPipelineInputAssemblyStateCreateFlags flags, 
			VkBool32 primitiveRestartEnable) 
		{
			VkPipelineInputAssemblyStateCreateInfo nInfo = {};
			nInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			nInfo.topology = topology;
			nInfo.flags = flags;
			nInfo.primitiveRestartEnable = primitiveRestartEnable;

			return nInfo;
		}

		VkPipelineRasterizationStateCreateInfo PipelineRasterizationStateCreateInfo(
			VkPolygonMode polygonMode, 
			VkCullModeFlags cullMode, 
			VkFrontFace frontFace, 
			VkPipelineRasterizationStateCreateFlags flags) 
		{
			VkPipelineRasterizationStateCreateInfo nInfo = {};
			nInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			nInfo.polygonMode = polygonMode;
			nInfo.cullMode = cullMode;
			nInfo.frontFace = frontFace;
			nInfo.flags = flags;
			nInfo.depthClampEnable = VK_FALSE;
			nInfo.lineWidth = 1.0f;

			return nInfo;
		}


		VkPipelineColorBlendAttachmentState PipelineColorBlendAttachmentState(VkColorComponentFlags colorWriteMask, VkBool32 blendEnable) 
		{
			VkPipelineColorBlendAttachmentState nState = {};
			nState.colorWriteMask = colorWriteMask;
			nState.blendEnable = blendEnable;
			return nState;
		}

		VkPipelineColorBlendStateCreateInfo PipelineColorBlendStateCreateInfo(uint32_t attachmentCount, VkPipelineColorBlendAttachmentState* pAttachments) 
		{
			VkPipelineColorBlendStateCreateInfo nInfo = {};
			nInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			nInfo.attachmentCount = attachmentCount;
			nInfo.pAttachments = pAttachments;
			return nInfo;
		}

		VkPipelineDepthStencilStateCreateInfo PipelineDepthStencilStateCreateInfo(VkBool32 depthTestEnable, VkBool32 depthWriteEnable, VkCompareOp depthCompareOp) 
		{
			VkPipelineDepthStencilStateCreateInfo nInfo = {};
			nInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			nInfo.depthTestEnable = depthTestEnable;
			nInfo.depthWriteEnable = depthWriteEnable;
			nInfo.depthCompareOp = depthCompareOp;
			nInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;
			return nInfo;
		}

		VkPipelineViewportStateCreateInfo PipelineViewportStateCreateInfo(uint32_t viewportCount, uint32_t scissorCount, VkPipelineViewportStateCreateFlags flags) 
		{
			VkPipelineViewportStateCreateInfo nInfo = {};
			nInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			nInfo.viewportCount = viewportCount;
			nInfo.scissorCount = scissorCount;
			return nInfo;
		}

		VkPipelineMultisampleStateCreateInfo PipelineMultisampleCreateInfo(VkSampleCountFlagBits rasterizationSamples, VkPipelineMultisampleStateCreateFlags flags) 
		{
			VkPipelineMultisampleStateCreateInfo nInfo = {};
			nInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			nInfo.flags = flags;
			nInfo.rasterizationSamples = rasterizationSamples;
			return nInfo;
		}

		VkPipelineDynamicStateCreateInfo PipelineDynamicStateCreateInfo(std::vector<VkDynamicState>& dynamicStates, VkPipelineDynamicStateCreateFlags flags) 
		{
			VkPipelineDynamicStateCreateInfo nInfo = {};
			nInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			nInfo.dynamicStateCount = dynamicStates.size();
			nInfo.pDynamicStates = dynamicStates.data();
			nInfo.flags = flags;
			return nInfo;
		}

		VkGraphicsPipelineCreateInfo PipelineCreateInfo(VkPipelineLayout pipelineLayout, VkRenderPass renderPass, VkPipelineCreateFlags flags) 
		{
			VkGraphicsPipelineCreateInfo nInfo = {};
			nInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			nInfo.layout = pipelineLayout;
			nInfo.renderPass = renderPass;
			nInfo.flags = flags;
			return nInfo;
		}

		VkPipeline CreateGraphicsPipeline(const VkDevice l_device, const VkPipelineLayout pipelineLayout, const VkRenderPass renderPass, VkPipelineShaderStageCreateInfo* pStages, int numStages, VkPrimitiveTopology primitiveTopology)
		{
			VkVertexInputBindingDescription vBindingDescription = vk::init::VertexInputBindingDescription();
			
			auto vAttribs = Vertex::InputAttributeDescriptions();

			VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = vk::init::PipelineVertexInputStateCreateInfo();
			vertexInputCreateInfo.vertexBindingDescriptionCount = 1; //vertexBindingDescriptionCount
			vertexInputCreateInfo.pVertexBindingDescriptions = &vBindingDescription,
				vertexInputCreateInfo.vertexAttributeDescriptionCount = vAttribs.size(); //attribute count
			vertexInputCreateInfo.pVertexAttributeDescriptions = vAttribs.data();

			VkPipelineInputAssemblyStateCreateInfo pipelineAssemblyCreateInfo = vk::init::PipelineInputAssemblyStateCreateInfo(primitiveTopology, 0, VK_FALSE);

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

			VkPipelineColorBlendAttachmentState			colorBlendAttachState = {};
			//this is equivalent to 0xf because all bits are enabled. 
			//1 + 2 + 4 + 8 = 15
			colorBlendAttachState.colorWriteMask = 0xf;
			colorBlendAttachState.blendEnable = VK_FALSE;

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
				VK_FORMAT_B8G8R8A8_UNORM, //normalized format --> 0-1 unsigned float.
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

		VkRenderPassBeginInfo RenderPassBeginInfo() 
		{
			VkRenderPassBeginInfo nInfo = {};
			nInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			return nInfo;
		}


		VkViewport Viewport(uint32_t width, uint32_t height, float minDepth, float maxDepth) 
		{
			VkViewport nViewPort = {};
			nViewPort.width = (float)width;
			nViewPort.height = (float)height;
			nViewPort.minDepth = minDepth;
			nViewPort.maxDepth = maxDepth;
			return nViewPort;
		}

		VkRect2D Rect2D(uint32_t width, uint32_t height, int32_t offset_x, int32_t offset_y) 
		{
			VkRect2D nRect = {};

			nRect.extent = { width, height };
			nRect.offset = { offset_x, offset_y };

			return nRect;

		}

		VkBufferCreateInfo BufferCreateInfo(VkBufferUsageFlags usageFlags, VkDeviceSize size) {

			VkBufferCreateInfo nInfo = {};
			nInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			nInfo.usage = usageFlags;
			nInfo.size = size;

			return nInfo;
		}

		VkCommandBufferBeginInfo CommandBufferBeginInfo() 
		{
			VkCommandBufferBeginInfo nInfo = {};
			nInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			return nInfo;
		}

	}
}
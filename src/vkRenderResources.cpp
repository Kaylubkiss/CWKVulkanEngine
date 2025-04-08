#include "vkRenderResources.h"
#include "ApplicationGlobal.h"
#include "vkInit.h"
#ifdef _DEBUG
#include "SpirvHelper.h"
#endif
#include "vkUtility.h"


namespace vk 
{
	namespace global 
	{
		uTransformObject uTransform = {};
		vk::Buffer uniformBuffer;
	}



	void RenderResources::Allocate(const VkPhysicalDevice p_device, const VkDevice l_device, const vk::Window& appWindow)
	{
		assert(_Application != NULL);

		Camera& appCamera = _Application->GetCamera();

		this->commandPool = vk::init::CommandPool(l_device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		this->commandBuffer = vk::init::CommandBuffer(l_device, this->commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

		//uniform transform for objects of default pipeline.
		global::uTransform = {
			glm::mat4(1.f), //model
			appCamera.LookAt(), //view
			glm::perspective(glm::radians(45.f), (float)appWindow.viewport.width / appWindow.viewport.height, 0.1f, 1000.f) //proj
		};

		global::uTransform.proj[1][1] *= -1.f;

		//uniform(s)
		global::uniformBuffer = vk::Buffer(p_device, l_device, sizeof(uTransformObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, (void*)&global::uTransform);

		//semaphores
		this->imageAvailableSemaphore = vk::init::CreateSemaphore(l_device);
		this->renderFinishedSemaphore = vk::init::CreateSemaphore(l_device);

		//fence(s)
		this->inFlightFence = vk::init::CreateFence(l_device);

		this->depthInfo = vk::rsc::CreateDepthResources(p_device, l_device, appWindow.viewport);

		this->renderPass = vk::init::RenderPass(l_device, this->depthInfo.depthFormat);

		//window sizing...
		VkSurfaceCapabilitiesKHR deviceCapabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(p_device, appWindow.surface, &deviceCapabilities);

		this->currentExtent = deviceCapabilities.currentExtent;
	}


	//void RenderResources::RecreatePipeline(const VkPhysicalDevice p_device, const VkDevice l_device, const vk::Window& appWindow)
	//{

	//	//create pipeline
	//	this->defaultDescriptorSetLayout = vk::init::DescriptorSetLayout(l_device);

	//	this->defaultPipelineLayout = vk::init::CreatePipelineLayout(l_device, this->defaultDescriptorSetLayout);

	//	//------------------create shader modules---------------
	//	std::string vertexShaderPath = std::string(SHADER_PATH) + "blinnvert.spv";
	//	std::string fragmentShaderPath = std::string(SHADER_PATH) + "blinnfrag.spv";


	//	//vertex shader reading and compilation
	//	vk::shader::CompilationInfo shaderInfo = {};
	//	shaderInfo.source = vk::util::ReadFile(std::string(SHADER_PATH) + "blinn.vert");
	//	shaderInfo.filename = "blinn.vert";
	//	shaderInfo.kind = shaderc_vertex_shader;

	//	std::vector<uint32_t> output = vk::shader::SourceToSpv(shaderInfo);

	//	vk::util::WriteSpirvFile(vertexShaderPath.data(), output);

	//	this->vertexShaderModule = vk::init::ShaderModule(l_device, vertexShaderPath.data());

	//	//fragment shader reading and compilation
	//	shaderInfo.source = vk::util::ReadFile(std::string(SHADER_PATH) + "blinn.frag");
	//	shaderInfo.filename = "blinn.frag";
	//	shaderInfo.kind = shaderc_fragment_shader;

	//	output = vk::shader::SourceToSpv(shaderInfo);

	//	vk::util::WriteSpirvFile(fragmentShaderPath.data(), output);

	//	this->fragmentShaderModule = vk::init::ShaderModule(l_device, fragmentShaderPath.data());

	//	VkPipelineShaderStageCreateInfo vertexShaderStageCreateInfo = vk::init::PipelineShaderStageCreateInfo(this->vertexShaderModule, VK_SHADER_STAGE_VERTEX_BIT);
	//	VkPipelineShaderStageCreateInfo fragmentShaderStageCreateInfo = vk::init::PipelineShaderStageCreateInfo(this->fragmentShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT);

	//	VkPipelineShaderStageCreateInfo shaderStages[2] = { vertexShaderStageCreateInfo, fragmentShaderStageCreateInfo };

	//	this->depthInfo = vk::rsc::CreateDepthResources(p_device, l_device, appWindow.viewport);

	//	this->renderPass = vk::init::RenderPass(l_device, depthInfo.depthFormat);

	//	this->defaultPipeline = vk::init::CreatePipeline(l_device, this->defaultPipelineLayout, this->renderPass, shaderStages, 2, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

	//}

	void RenderResources::Destroy(const VkDevice l_device)
	{
		//pipeline info...

		vkDestroyRenderPass(l_device, this->renderPass, nullptr);

		this->depthInfo.Destroy(l_device);

		vkDestroyFence(l_device, this->inFlightFence, nullptr);

		//command pools and their handles.
		vkFreeCommandBuffers(l_device, this->commandPool, 1, &this->commandBuffer);
		vkDestroyCommandPool(l_device, this->commandPool, nullptr);

		//semaphores
		vkDestroySemaphore(l_device, this->imageAvailableSemaphore, nullptr);
		vkDestroySemaphore(l_device, this->renderFinishedSemaphore, nullptr);

		//buffers
		vkDestroyBuffer(l_device, global::uniformBuffer.handle, nullptr);
		vkFreeMemory(l_device, global::uniformBuffer.memory, nullptr);
	}
}

#include "vkPipeline.h"
#include "vkUtility.h"
#include "vkInit.h"
#include <iostream>


namespace vk 
{

	//Initializers for shaderModule changes depending on debug and release!
	// This is to help reduce the size of the application and its objects.
#ifdef _DEBUG
	void Pipeline::AddModule(const VkDevice l_device, const std::string& filename, VkShaderStageFlagBits shaderFlags, shaderc_shader_kind shader_kind)
	{
	
		
			std::string shaderPath = vk::util::ReadSourceAndWriteToSprv(filename, shader_kind);
			if (shaderPath.empty())
			{
				std::cerr << "[ERROR] Couldn't successfully read shader file " << filename << '\n';
				return;
			}
			shaderModules.push_back({ SHADER_PATH + filename, shader_kind,
									vk::init::ShaderModule(l_device, shaderPath.data()), 
									shaderFlags });	
	}
#else
	void Pipeline::AddModule(const VkDevice l_device, const std::string& filename, VkShaderStageFlagBits shaderFlags) 
	{
		std::string shaderPath = SHADER_PATH + filename;
		shaderModules.push_back({ vk::init::ShaderModule(l_device, shaderPath.data()),
								  shaderFlags });
	}
#endif



	void Pipeline::Recreate(const VkDevice l_device, const VkRenderPass renderPass) 
	{
		//NOTE: globals, i.e. descriptor set layout 
		// cannot be edited in the shader during runtime!!
		vkDestroyPipeline(l_device, this->handle, nullptr);


		std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfo;

		for (size_t i = 0; i < shaderModules.size(); ++i) {
			shaderStageCreateInfo.push_back(vk::init::PipelineShaderStageCreateInfo(shaderModules[i].handle, shaderModules[i].flags));
		}

		vk::init::CreatePipeline(l_device, this->layout, renderPass,
			shaderStageCreateInfo.data(), shaderStageCreateInfo.size(),
			this->mTopology);
	}

	void Pipeline::Finalize(const VkDevice l_device, const VkRenderPass renderPass, VkPrimitiveTopology topology) 
	{
		this->descriptorSetLayout = vk::init::DescriptorSetLayout(l_device);

		this->layout = vk::init::CreatePipelineLayout(l_device, this->descriptorSetLayout);

		this->mTopology = topology;

		std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfo;

		for (size_t i = 0; i < shaderModules.size(); ++i){
			shaderStageCreateInfo.push_back(vk::init::PipelineShaderStageCreateInfo(shaderModules[i].handle, shaderModules[i].flags));
		}

		this->handle = vk::init::CreatePipeline(l_device, this->layout, renderPass,
												shaderStageCreateInfo.data(), shaderStageCreateInfo.size(), 
												this->mTopology);
	}

	void Pipeline::Destroy(const VkDevice l_device) 
	{

		vkDestroyDescriptorSetLayout(l_device, this->descriptorSetLayout, nullptr);
		vkDestroyPipelineLayout(l_device, this->layout, nullptr);
		vkDestroyPipeline(l_device, this->handle, nullptr);


		for (size_t i = 0; i < shaderModules.size(); ++i) {
			vkDestroyShaderModule(l_device, shaderModules[i].handle, nullptr);
		}

	}

	const VkDescriptorSetLayout Pipeline::DescriptorSetLayout() const {
		return this->descriptorSetLayout;
	}

	const VkPipeline Pipeline::Handle() const {
		return this->handle;
	}

	const VkPipelineLayout Pipeline::Layout() const {
		return this->layout;
	}

	std::vector<ShaderModuleInfo>& Pipeline::ShaderModules() 
	{
		return this->shaderModules;
	}


}
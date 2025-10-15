/*
	* filename: VkPipelineManager.cpp
	* author: Caleb Kissinger
*/
#include "vkPipelineManager.h"
#include "vkUtility.h"
#include "vkInit.h"
#include <string> //std::sting
#include <iostream> //std::cerr
#include <sys/stat.h>

//ShaderModuleInfo 
namespace vk 
{
	ShaderModuleInfo::ShaderModuleInfo(const VkDevice l_device, std::string filename, VkShaderStageFlagBits shaderFlags, shaderc_shader_kind shaderc_kind) : mFlags(shaderFlags), mShaderKind(shaderc_kind)
	{

		std::string shaderPath = vk::util::ReadSourceAndWriteToSprv(SHADER_PATH + filename, shaderc_kind);
	
		if (shaderPath.empty())
		{
			std::cerr << "[ERROR] Couldn't successfully read shader file " << filename << '\n';
			return;
		}
		else 
		{
			mFilePath = SHADER_PATH + filename;
		}

		mHandle = vk::init::ShaderModule(l_device, shaderPath.data());

		struct stat fileStat;
		if (stat(mFilePath.c_str(), &fileStat) != 0)
		{
			std::cerr << "[ERROR] Can't open file: " << mFilePath << '\n';
		}
		else
		{
			lastModificationTime = fileStat.st_mtime;
		}
	}

}

//PipelineManager
namespace vk 
{

	void PipelineManager::Init(const GraphicsContextInfo& contextInfo)
	{
		contextLogicalDevice = contextInfo.logicalDevice;
	}

	void PipelineManager::Destroy()
	{
		for (auto& pipeline : pipelines)
		{
			vk::Pipeline& currPipeline = pipeline.second;

			vkDestroyPipeline(contextLogicalDevice, currPipeline.handle, nullptr);

			for (auto& shaderModule : currPipeline.shaderModules)
			{
				vkDestroyShaderModule(contextLogicalDevice, shaderModule.mHandle, nullptr);
			}
		}
	}

	void PipelineManager::AddModule(uint32_t pipeline, const ShaderModuleInfo& shaderModuleInfo)
	{
		pipelines[pipeline].shaderModules.push_back(shaderModuleInfo);
	}

	void PipelineManager::AddPipeline(uint32_t pipeline, const VkPipeline handle, std::function<void()> createFunc)
	{
		pipelines[pipeline].handle = handle;

		if (createFunc != nullptr) 
		{
			pipelines[pipeline].createFunc = std::move(createFunc);
		}
	}

	//TODO: remove this entirely, only serviced the freddy head scene.
	void PipelineManager::Finalize(const VkDevice l_device, const VkPhysicalDevice p_device, const vk::Window& appWindow, VkPrimitiveTopology topology)
	{
		/*this->mTopology = topology;

		std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfo;

		for (size_t i = 0; i < shaderModules.size(); ++i){
			shaderStageCreateInfo.push_back(vk::init::PipelineShaderStageCreateInfo(shaderModules[i].mHandle, shaderModules[i].mFlags));
		}

		this->handle = vk::init::CreateGraphicsPipeline(l_device, this->layout, this->mRenderPass,
												shaderStageCreateInfo.data(), 
												shaderStageCreateInfo.size(), 
												this->mTopology);

		return *this;*/
	}

	void PipelineManager::HotReloadShaders() 
	{
		bool somethingChanged = false;

		for (auto& pipeline : pipelines) 
		{
			for (auto& shader : pipeline.second.shaderModules) 
			{
				struct stat fileStat;

				if (stat(shader.mFilePath.c_str(), &fileStat) != 0)
				{
					std::cerr << "[ERROR] Can't Read File " << shader.mFilePath << '\n';
					continue;
				}

				//check if the filesystem saw any changes.
				if (fileStat.st_mtime != shader.lastModificationTime)
				{
					std::string shaderPath =
						vk::util::ReadSourceAndWriteToSprv(shader.mFilePath, shader.mShaderKind);

					if (shaderPath.empty())
					{
						std::cerr << "[ERROR] Couldn't successfully write to file " << shader.mFilePath << '\n';
						continue;
					}

					somethingChanged = true;

					VK_CHECK_RESULT(vkDeviceWaitIdle(contextLogicalDevice))

					vkDestroyShaderModule(contextLogicalDevice, shader.mHandle, nullptr);
					shader.mHandle = vk::init::ShaderModule(contextLogicalDevice, shaderPath.data());

					shader.lastModificationTime = fileStat.st_mtime;
				}	
			}

			if (somethingChanged)
			{
				if (pipeline.second.createFunc != nullptr) 
				{
					pipeline.second.createFunc();
				}

				somethingChanged = false;
			}

		}

	}

	VkPipeline PipelineManager::Get(uint32_t pipeline) 
	{
		return pipelines[pipeline].handle;
	}

	const std::vector<ShaderModuleInfo>& PipelineManager::GetPipelineShaders(uint32_t pipeline) 
	{
		return pipelines[pipeline].shaderModules;
	}


	 



}
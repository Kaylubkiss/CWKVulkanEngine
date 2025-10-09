/*
	* filename: VkPipeline.cpp
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

			mFilePath = shaderPath.c_str();
		}

		mHandle = vk::init::ShaderModule(l_device, shaderPath.data());

		struct stat fileStat;
		if (stat(mFilePath, &fileStat) != 0)
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

	void PipelineManager::AddPipeline(uint32_t pipeline, const VkPipeline handle) 
	{
		pipelines[pipeline].handle = handle;
	}


	//TODO
	void PipelineManager::Recreate(const VkDevice l_device) 
	{
		//NOTE: globals, i.e. descriptor set layout 
		// cannot be edited in the shader during runtime!!
		/*vkDestroyPipeline(l_device, this->handle, nullptr);


		std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfo;

		for (size_t i = 0; i < shaderModules.size(); ++i) {
			shaderStageCreateInfo.push_back(vk::init::PipelineShaderStageCreateInfo(shaderModules[i].mHandle, shaderModules[i].mFlags));
		}

		this->handle = vk::init::CreateGraphicsPipeline(l_device, this->layout, this->mRenderPass,
			shaderStageCreateInfo.data(), shaderStageCreateInfo.size(),
			this->mTopology);*/
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
		//bool somethingChanged = false;
		//const VkDevice& appDevice = (this->appDevicePtr);


		//for (size_t i = 0; i < shaderInfos.size(); ++i)
		//{
		//	struct stat fileStat;

		//	vk::ShaderModuleInfo& shaderModule = pipelinePtr->ShaderModule(shaderInfos[i].module_i);

		//	std::string filePath = SHADER_PATH + shaderModule.mFileName;

		//	if (stat(filePath.c_str(), &fileStat) != 0)
		//	{
		//		std::cerr << "[ERROR] Can't Read File " << filePath << '\n';
		//		continue;
		//	}

		//	//check if the filesystem saw any changes.
		//	if (fileStat.st_mtime != shaderInfos[i].last_modification)
		//	{
		//		std::string shaderPath =
		//			vk::util::ReadSourceAndWriteToSprv(filePath, shaderModule.mShaderKind);

		//		if (shaderPath.empty())
		//		{
		//			std::cerr << "[ERROR] Couldn't successfully write to file " << shaderModule.mFileName << '\n';
		//			continue;
		//		}

		//		somethingChanged = true;

		//		VK_CHECK_RESULT(vkDeviceWaitIdle(appDevicePtr))

		//			vkDestroyShaderModule(appDevice, shaderModule.mHandle, nullptr);
		//		shaderModule.mHandle = vk::init::ShaderModule(appDevice, shaderPath.data());

		//		shaderInfos[i].last_modification = fileStat.st_mtime;
		//	}
		//}

		//if (somethingChanged)
		//{
		//	pipelinePtr->Recreate(appDevice);
		//}

	}


	VkPipeline PipelineManager::Get(uint32_t pipeline) 
	{
		return pipelines[pipeline].handle;
	}


	 



}
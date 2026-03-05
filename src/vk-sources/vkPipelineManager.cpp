/*
	* filename: VkPipelineManager.cpp
	* author: Caleb Kissinger
*/
#include "vkPipelineManager.h"
#include "SpirvHelper.h"
#include <sys/stat.h>

#define SHADER_PATH "shaders/"

//ShaderModuleInfo 
namespace vk 
{
	ShaderModuleInfo::ShaderModuleInfo( const VkDevice l_device, std::string_view filename,
		VkShaderStageFlagBits shaderFlags )
	{
		mFlags = shaderFlags;

		switch (shaderFlags)
		{
			case VK_SHADER_STAGE_VERTEX_BIT:
				mShaderKind = shaderc_vertex_shader;
				break;
			case VK_SHADER_STAGE_FRAGMENT_BIT:
				mShaderKind = shaderc_fragment_shader;
				break;
			case VK_SHADER_STAGE_GEOMETRY_BIT:
				mShaderKind = shaderc_geometry_shader;
				break;
			default:
				throw std::runtime_error("Unknown shader kind");
		}

		std::string sourceFilePath = SHADER_PATH + std::string(filename);
		auto spirvPath = vk::spirv::ReadSourceAndWriteToSpirv(sourceFilePath, mShaderKind);
	
		if (spirvPath.has_value() == false)
		{			
			return;
		}
		else 
		{
			mFilePath = sourceFilePath;
		}

		mHandle = vk::init::ShaderModule(l_device, spirvPath.value().c_str());

		struct stat fileStat = {};
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

	void PipelineManager::Init(std::shared_ptr<GraphicsContextInfo>& contextInfo)
	{
		assert(contextInfo->devicePtr);

		contextLogicalDevice = contextInfo->devicePtr->GetDevice();
	}

	void PipelineManager::Destroy()
	{
		if (contextLogicalDevice != VK_NULL_HANDLE)
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
	}

	void PipelineManager::AddModule( uint32_t pipeline, const ShaderModuleInfo& shaderModuleInfo )
	{
		pipelines[pipeline].shaderModules.push_back(shaderModuleInfo);
	}

	void PipelineManager::AddPipeline( uint32_t pipeline, const VkPipeline handle, std::function<void()>&& createFunc )
	{
		pipelines[pipeline].handle = handle;

		if (createFunc != nullptr) 
		{
			pipelines[pipeline].createFunc = std::move(createFunc);
		}
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
					shader.lastModificationTime = fileStat.st_mtime; //this is so errors don't pop up forever.

					auto shaderPath =
						vk::spirv::ReadSourceAndWriteToSpirv(shader.mFilePath, shader.mShaderKind, true);

					if (shaderPath.has_value() == false)
					{
						std::cerr << "[ERROR] Couldn't successfully write to file " << shader.mFilePath << '\n';

						continue;
					}

					somethingChanged = true;

					VK_CHECK_RESULT(vkDeviceWaitIdle(contextLogicalDevice));

					vkDestroyShaderModule(contextLogicalDevice, shader.mHandle, nullptr);
					shader.mHandle = vk::init::ShaderModule(contextLogicalDevice, shaderPath.value().c_str());
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

	VkPipeline PipelineManager::Get( uint32_t pipeline )
	{
		return pipelines[pipeline].handle;
	}

	const std::vector<ShaderModuleInfo>& PipelineManager::GetPipelineShaders( uint32_t pipeline )
	{
		return pipelines[pipeline].shaderModules;
	}
}
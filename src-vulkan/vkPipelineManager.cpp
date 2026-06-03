/*
	* filename: VkPipelineManager.cpp
	* author: Caleb Kissinger
*/
#include "vkPipelineManager.h"
#include "vkInit.h"
#include <sys/stat.h>

//PipelineManager
namespace vk 
{

	PipelineManager::PipelineManager( const GraphicsContextInfo& contextInfo )
	{
		assert(contextInfo.devicePtr != nullptr);

		contextLogicalDevice = contextInfo.devicePtr->GetDevice();
	}

	PipelineManager::PipelineManager( PipelineManager&& other ) noexcept
	{
		this->contextLogicalDevice = other.contextLogicalDevice;
		this->hotReloadInfos = other.hotReloadInfos;
		this->pipelines = std::move(other.pipelines);

		other.contextLogicalDevice = VK_NULL_HANDLE;
	}

	PipelineManager& PipelineManager::operator=( PipelineManager&& other ) noexcept
	{
		if (this != &other)
		{
			std::swap(this->contextLogicalDevice, other.contextLogicalDevice);
			std::swap(this->hotReloadInfos, other.hotReloadInfos);
			std::swap(this->pipelines, other.pipelines);
		}

		return *this;
	}

	PipelineManager::~PipelineManager()
	{
		if (contextLogicalDevice != VK_NULL_HANDLE)
		{
			for (auto& pipeline : pipelines)
			{
				vk::Pipeline& currPipeline = pipeline.second;

				vkDestroyPipeline(contextLogicalDevice, currPipeline.handle, nullptr);
			}

			pipelines.clear();
		}
	}

	void PipelineManager::AddModule( uint32_t pipeline, ShaderModuleInfo&& shaderModuleInfo )
	{
		pipelines[pipeline].shaderModules.push_back(std::move(shaderModuleInfo));
	}

	void PipelineManager::AddPipeline( uint32_t pipeline, const VkPipeline handle, std::function<void()>&& createFunc )
	{
		pipelines[pipeline].handle = handle;

		if (createFunc != nullptr) 
		{
			pipelines[pipeline].createFunc = std::move(createFunc);
		}
	}


	//recreate the pipeline and its marked modules
	void PipelineManager::HotReloadShaders() 
	{
		VK_CHECK_RESULT(vkDeviceWaitIdle(contextLogicalDevice));

		for ( auto& hInfo : hotReloadInfos )
		{
			HotReloadInfo& info = hInfo.second;

			Pipeline& pipeline = pipelines[info.pipeline_index];

			vkDestroyPipeline(contextLogicalDevice,  pipeline.handle, nullptr);
			pipeline.handle = VK_NULL_HANDLE;

			for ( auto& module : info.modules )
			{
				ShaderModuleInfo& shaderModule = pipeline.shaderModules[module.index];

				shaderModule = vk::ShaderModuleInfo( contextLogicalDevice,
					shaderModule.GetFileName(), shaderModule.GetShaderStageFlags() );
			}

			pipeline.createFunc();
		}

		hotReloadInfos.clear();
	}

	//helper for DetectHotReloadableShaders()
	inline HotReloadInfo CheckPipelineShaderChanges( uint32_t pipelineIndex, vk::Pipeline& pipeline )
	{
		HotReloadInfo info;

		//avoid any unecessary reallocations.
		info.modules.reserve(pipeline.shaderModules.size());

		for ( size_t i = 0; i < pipeline.shaderModules.size(); ++i )
		{
			ShaderModuleInfo& shader = pipeline.shaderModules[i];

			struct stat fileStat = {};

			const auto& shaderFileName = SHADER_PATH + shader.GetFileName();

			if (stat(shaderFileName.c_str(), &fileStat) != 0)
			{
				std::cerr << "[ERROR] Can't Read File " << shaderFileName << '\n';
				continue;
			}

			//check if the filesystem saw any changes.
			if (shader.GetModificationTime() != fileStat.st_mtime)
			{
				//reassign so errors don't pop up forever.
				shader.SetModificationTime( fileStat.st_mtime );

				//creation function determines that a pipeline can be hot reloaded.
				if (pipeline.createFunc != nullptr)
				{
					info.pipeline_index = static_cast<int>( pipelineIndex );

					auto shaderPath =
						vk::spirv::ReadSourceAndWriteToSpirv(shaderFileName, shader.GetShaderKind(), true);

					if (shaderPath.has_value() == false)
					{
						std::cerr << "[ERROR] Couldn't write to file " << shaderFileName << '\n';
						continue;
					}

					info.modules.push_back({i, shaderPath.value()});
				}
				else
				{
					std::cerr << "no creation function provided for pipeline " << '\n';
				}
			}
		}

		return info;
	}

	void PipelineManager::DetectHotReloadableShaders()
	{
		for (auto& pipeline : pipelines)
		{
			HotReloadInfo&& info = CheckPipelineShaderChanges( pipeline.first, pipeline.second );
			if (info.pipeline_index >= 0)
			{
				hotReloadInfos[info.pipeline_index] = info;
			}
		}
	}

	bool PipelineManager::HotReloadIsReady() const
	{
		return hotReloadInfos.empty() == false;
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
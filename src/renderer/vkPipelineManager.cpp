/*
	* filename: VkPipelineManager.cpp
	* author: Caleb Kissinger
*/
#include "vkPipelineManager.h"
#include "vkShaderModule.h"
#include <sys/stat.h>

//PipelineManager
namespace vk 
{

	PipelineManager::PipelineManager( const vk::Device& device )
	{
		contextLogicalDevice = device.GetDevice();
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

	void PipelineManager::AddPipeline( uint32_t pipeline, Pipeline& pipelineInfo )
	{
		pipelines[pipeline].pipelineBuilder = std::move(pipelineInfo.pipelineBuilder);
		pipelines[pipeline].handle = pipelineInfo.handle;
	}


	//recreate the pipeline and its marked modules
	void PipelineManager::HotReloadShaders() 
	{
		VK_CHECK_RESULT( vkDeviceWaitIdle(contextLogicalDevice) );

		for ( auto& hInfo : hotReloadInfos )
		{
			HotReloadInfo& info = hInfo.second;

			Pipeline& pipeline = pipelines[info.pipeline_index];

			vkDestroyPipeline(contextLogicalDevice,  pipeline.handle, nullptr);
			pipeline.handle = VK_NULL_HANDLE;

			auto& shaderModules = pipeline.pipelineBuilder->GetShaderModules();

			for ( auto& module : info.moduleInfos )
			{
				shaderModules[module.index] = vk::ShaderModuleInfo( contextLogicalDevice,
					shaderModules[module.index].GetFileName(),
					shaderModules[module.index].GetShaderStageFlags() );
			}

			pipeline.pipelineBuilder->CreatePipeline( contextLogicalDevice, &pipeline.handle );
		}

		hotReloadInfos.clear();
	}

	//helper for DetectHotReloadableShaders()
	inline HotReloadInfo CheckPipelineShaderChanges( uint32_t pipelineIndex, vk::Pipeline& pipeline )
	{

		HotReloadInfo info;

		if (pipeline.pipelineBuilder != nullptr)
		{
			//avoid any unecessary reallocations.
			auto& shaderModules = pipeline.pipelineBuilder->GetShaderModules();

			size_t shaderCount = shaderModules.size();

			info.moduleInfos.reserve(shaderCount);

			for ( size_t i = 0; i < shaderCount; ++i )
			{
				std::string shaderFileName = SHADER_PATH + shaderModules[i].GetFileName();

				struct stat fileStat = {};
				if (stat(shaderFileName.c_str(), &fileStat) != 0)
				{
					std::cerr << "[ ERROR ] Can't read File " << shaderFileName << '\n';
					continue;
				}

				//check if the filesystem saw any changes.
				if (shaderModules[i].GetModificationTime() != fileStat.st_mtime)
				{
					//reassign so errors don't pop up forever.
					shaderModules[i].SetModificationTime( fileStat.st_mtime );

					info.pipeline_index = static_cast<int>( pipelineIndex );

					std::string shaderPath =
						vk::spirv::ReadSourceAndWriteToSpirv(shaderFileName,
							shaderModules[i].GetShaderKind(), true);

					if (shaderPath.empty())
					{
						std::cerr << "[ ERROR ] Couldn't read/write to file " << shaderFileName << '\n';
						continue;
					}

					info.moduleInfos.push_back({i, shaderPath});
				}
			}
		}
		else
		{
			std::cerr << "no builder provided for this pipeline, can't hot reload\n";
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

	std::unique_ptr<vk::PipelineBuilder>& PipelineManager::GetPipelineManager( uint32_t pipeline )
	{
		return pipelines[pipeline].pipelineBuilder;
	}
}
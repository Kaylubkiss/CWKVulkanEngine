#include "vkDeferredShadingContext.h"


namespace vk 
{

	DeferredContext::DeferredContext() 
	{


	}


	DeferredContext::~DeferredContext() 
	{


	}

	void DeferredContext::InitializeScene(ObjectManager& objManager) 
	{


	}

	void DeferredContext::InitializeDescriptors() 
	{


	}
	
	void DeferredContext::InitializePipeline(std::string vsFile, std::string fsFile)
	{

	}

	void DeferredContext::RecordCommandBuffers(vk::ObjectManager& objManager)
	{


	}


	const VkDescriptorSetLayout DeferredContext::DescriptorSetLayout() const 
	{
		return sceneDescriptorSetLayout;
	}

	std::vector<VkWriteDescriptorSet> DeferredContext::WriteDescriptorBuffers(VkDescriptorSet descriptorSet) 
	{
		//TODO: no textures in this scene.. yet.
		return {};
	}

	uint32_t DeferredContext::SamplerDescriptorSetBinding() 
	{
		//TODO: no textures in this scene.. yet.
		return 0;
	}

	



}
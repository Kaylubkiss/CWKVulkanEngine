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

	void DeferredContext::InitializeDeferredRenderPass()
	{
		VkRenderPassCreateInfo createInfo = vk::init::RenderPassCreateInfo();

		VkAttachmentReference colorReferences[3] =
		{
			{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}, //position
			{1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}, //normals
			{2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL} //albedo
		};

		VkAttachmentReference depthReference = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };


		VkSubpassDependency subpass[2] = {};
		
		subpass[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		subpass[0].dstSubpass = 0;
		subpass[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpass[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpass[0].srcAccessMask = 0;
		subpass[0].dstAccessMask = 0;
		//create two subpasses to represent the geometry pass and light pass.
		//establish dependencies between the two --> g-pass is index 0, and l-pass is index 1
		// 
		//this->deferredRenderPass;

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
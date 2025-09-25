#pragma once

#include "Common.h"
#include "vkBuffer.h"

namespace vk {

	/* stores information unique to shadow mapping. */
	class ShadowMap 
	{
		float zNear = 1.0f;
		float zFar = 96.0f;

		// Depth bias (and slope) are used to avoid shadowing artifacts
		// Constant depth bias factor (always applied)
		float depthBiasConstant = 1.25f;

		// Slope depth bias factor, applied depending on polygon's slope
		float depthBiasSlope = 1.75f;

		float lightFOV = 45.0f;

		struct uShadowMap 
		{
			glm::mat4 uDepthMVP;
			glm::mat4 uDepthBiasMVP;
		};

		vk::Buffer uOffscreenBuffer;

		struct FrameBufferAttachment
		{
			VkImage image;
			VkDeviceMemory mem;
			VkImageView view;
		};

		struct OffscreenPass {
			int32_t width, height;
			VkFramebuffer frameBuffer;
			FrameBufferAttachment depth;
			VkRenderPass renderPass;
			VkSampler depthSampler;
			VkDescriptorImageInfo descriptor;
		} offscreenPass{};

		const uint32_t shadowMapize{ 2048 };

	};
}

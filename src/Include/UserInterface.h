#pragma once

#include "Common.h"

namespace vk 
{
	class UserInterface {
		public:
			UserInterface();
			~UserInterface();

		private:
			VkDescriptorPool UIDescriptorPool; //just for the sampler.

	};

}
#pragma once

#include "Common.h"

namespace vk 
{
	namespace tutorial 
	{
		#define NUM_PARTICLES 100

		struct Particle 
		{
			glm::vec2 position = glm::vec2(0.0);
			glm::vec3 velocity = glm::vec3(0.0);
			glm::vec4 color = glm::vec4(0.0);
		};
	}

}
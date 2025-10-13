#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

struct Vertex
{
	glm::vec3 pos = { 0,0,0 };
	glm::vec3 nrm = { .2f,.5f,0 };
	glm::vec2 uv = { -1.f,-1.f };

	bool operator==(const Vertex& other) const {
		return pos == other.pos && nrm == other.nrm && uv == other.uv;
	}

	static inline std::array<VkVertexInputAttributeDescription, 3> InputAttributeDescriptions() 
	{
		//TODO: check VkPhysicalDeviceLimits!!! 
			// binding -> maxVertexInputBindings, 
			// location -> maxVertexInputAttributes
			// offset -> maxVertexInputAttributeOffset
			//
		std::array<VkVertexInputAttributeDescription, 3> vInputAttribute = {};

		//position	
		vInputAttribute[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		vInputAttribute[0].location = 0;
		vInputAttribute[0].binding = 0;
		vInputAttribute[0].offset = offsetof(struct Vertex, pos);

		//normal
		vInputAttribute[1].format = vInputAttribute[0].format;
		vInputAttribute[1].location = 1;
		vInputAttribute[1].binding = 0;
		vInputAttribute[1].offset = offsetof(struct Vertex, nrm);

		//texture 
		vInputAttribute[2].format = VK_FORMAT_R32G32_SFLOAT;
		vInputAttribute[2].location = 2;
		vInputAttribute[2].binding = 0;
		vInputAttribute[2].offset = offsetof(struct Vertex, uv);

		return vInputAttribute;

	}
};

namespace std {
	template<>
	struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const
		{
			size_t h1 = hash<glm::vec3>{}(vertex.pos);
			size_t h2 = hash<glm::vec3>{}(vertex.nrm);
			size_t h3 = hash<glm::vec2>{}(vertex.uv);

			return ((h1 ^ (h2 << 1)) >> 1) ^ (h3 << 1);
		}
	};
}

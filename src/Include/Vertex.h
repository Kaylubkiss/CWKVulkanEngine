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

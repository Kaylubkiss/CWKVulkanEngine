#ifndef VERTEX_HPP
#define VERTEX_HPP

//NOTE: vec4s are placed at the bottom as opposed to top because vec3 and 4s are the same size (16-byte alignment).
struct Vertex
{
	glm::vec3 pos = { 0,0,0 }; //location = 0
	glm::vec3 nrm = { 0,0,0 }; //location = 1
	glm::vec2 uv = { 0,0 }; //location = 2
	glm::vec3 tan = {1,1,1}; //location = 3
	glm::vec3 bitan = {1,1,1}; //location = 4
	glm::vec4 jointIndices{}; //location = 5
	glm::vec4 jointWeights{}; //location = 6

	bool operator==(const Vertex& other) const
	{
		return pos == other.pos &&
			nrm == other.nrm &&
				uv == other.uv &&
					tan == other.tan &&
						bitan == other.bitan;
	}

	static std::vector<VkVertexInputAttributeDescription> InputAttributeDescriptions()
	{
		//TODO: check VkPhysicalDeviceLimits!!! 
		// binding -> maxVertexInputBindings,
		// location -> maxVertexInputAttributes
		// offset -> maxVertexInputAttributeOffset
		//
		return
		{
			{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(struct Vertex, pos) },
		{1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(struct Vertex, nrm) },
		{2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(struct Vertex, uv) },
		{3, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(struct Vertex, tan) },
		{ 4, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(struct Vertex, bitan) },
		{5, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(struct Vertex, jointIndices)},
		{6, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(struct Vertex, jointWeights)}
		};
	}
};

namespace std {
	template<>
	struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const noexcept {
			size_t h1 = hash<glm::vec3>{}(vertex.pos);
			size_t h2 = hash<glm::vec3>{}(vertex.nrm);
			size_t h3 = hash<glm::vec2>{}(vertex.uv);

			return ((h1 ^ (h2 << 1)) >> 1) ^ (h3 << 1);
		}
	};
}

#endif

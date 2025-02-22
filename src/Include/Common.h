#pragma once
//IF ENABLED: this means older versions of code compiled with mvsc from before 2017 will probably not be compatible.
//IF DISABLED: alignment may not be correct. For now, I'm willing to let this happen until something goes bad.


#define _DISABLE_EXTENDED_ALIGNED_STORAGE 
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>
#include "vkGlobal.h"


#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_sdl2.h"
#include "imgui/backends/imgui_impl_vulkan.h"

#include "Physics.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <SDL2/SDL.h>


class Application; //forward declare class.

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











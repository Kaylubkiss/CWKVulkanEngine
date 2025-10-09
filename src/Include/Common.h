#pragma once
//IF ENABLED: this means older versions of code compiled with mvsc from before 2017 will probably not be compatible.
//IF DISABLED: alignment may not be correct. For now, I'm willing to let this happen until something goes bad.


//#define _DISABLE_EXTENDED_ALIGNED_STORAGE 
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.h>

#include "vkGlobal.h"
#include "vkUtility.h"
#include "vkInit.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_sdl2.h"
#include "imgui/backends/imgui_impl_vulkan.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include "ApplicationGlobal.h"













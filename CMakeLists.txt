cmake_minimum_required(VERSION 3.2...3.5)

set(PROJECT_NAME "CKVulkan")
project("CKVulkan")

set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED)

#---------BUILD EXTERNAL LIBRARIES--------------
add_subdirectory(extern/fastgltf)
add_subdirectory(extern/reactphysics3d)

add_compile_definitions(GLM_ENABLE_EXPERIMENTAL)
add_compile_definitions(GLM_FORCE_DEPTH_ZERO_TO_ONE) #vulkan's NDC z-coordinate ranges from 0 to 1.
add_compile_definitions(GLM_FORCE_DEFAULT_ALIGNED_GENTYPES) #need to have aligned storage for shaders
add_subdirectory(extern/glm)

set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_OPENGL OFF CACHE BOOL "" FORCE)
set(SDL_OPENGLES OFF CACHE BOOL "" FORCE)
set(SDL_DISKAUDIO OFF CACHE BOOL "" FORCE)
set(SDL_DUMMYAUDIO OFF CACHE BOOL "" FORCE)
set(SDL_DUMMYVIDEO OFF CACHE BOOL "" FORCE)
set(SDL_ALTIVEC OFF CACHE BOOL "" FORCE)
set(SDL_WASAPI OFF CACHE BOOL "" FORCE)
set(SDL_VIRTUAL_JOYSTICK OFF CACHE BOOL "" FORCE)
set(SDL_RENDER_D3D OFF CACHE BOOL "" FORCE)
add_subdirectory(extern/sdl2)

set(SHADERC_SKIP_TESTS ON CACHE BOOL "" FORCE)
set(SHADERC_SKIP_EXAMPLES ON CACHE BOOL "" FORCE)
set(SHADERC_SKIP_COPYRIGHT_CHECK ON CACHE BOOL "" FORCE)
set(SHADERC_ENABLE_SHARED_CRT ON CACHE BOOL "" FORCE)
add_subdirectory(extern/shaderc)

#don't need to build these every time so set STATIC
add_library(imgui STATIC
		extern/imgui/imgui.cpp
		extern/imgui/imgui_tables.cpp
		extern/imgui/imgui_widgets.cpp
		extern/imgui/imgui_demo.cpp
		extern/imgui/imgui_draw.cpp
		#project uses SDL2 and Vulkan
		extern/imgui/backends/imgui_impl_vulkan.cpp
		extern/imgui/backends/imgui_impl_sdl2.cpp
)

target_include_directories(imgui PUBLIC
		extern/imgui
		extern/imgui/backends
)

target_link_libraries(imgui PRIVATE SDL2-static Vulkan::Vulkan)

#------------------CORE ENGINE DIRECTORIES----------------------
set(CORE_MANAGERS_DIR src_core/managers)
set(CORE_RESOURCES_DIR src_core/resources)

#------------------CORE ENGINE SOURCES----------------------
set(SOURCE_FILES
		src_core/Application.cpp
		src_core/Camera.cpp
		src_core/Main.cpp
		src_core/Object.cpp
		src_core/Physics.cpp
		src_core/Timer.cpp
		src_core/ThreadPool.cpp
		src_core/ApplicationGlobal.cpp
		src_core/CameraController.cpp
		src_core/UserInterface.cpp
		src_core/resources/GLTFModel.cpp
		src_core/resources/OBJModel.cpp
		src_core/managers/SceneManager.cpp
)


#------------------VULKAN IMPLEMENTATION SOURCES----------------------
set(VULKAN_SRC_DIR src_vulkan)

set(VULKAN_ENGINE_SRC 
${VULKAN_SRC_DIR}/vkUtility.cpp
${VULKAN_SRC_DIR}/vkInit.cpp
${VULKAN_SRC_DIR}/vkGlobal.cpp
${VULKAN_SRC_DIR}/vkContextBase.cpp
${VULKAN_SRC_DIR}/vkSwapChain.cpp
${VULKAN_SRC_DIR}/vkBuffer.cpp
${VULKAN_SRC_DIR}/vkTexture.cpp
${VULKAN_SRC_DIR}/vkDevice.cpp
${VULKAN_SRC_DIR}/vkWindow.cpp
${VULKAN_SRC_DIR}/vkPipelineManager.cpp
${VULKAN_SRC_DIR}/vkFramebuffer.cpp
${VULKAN_SRC_DIR}/vkInstance.cpp
${VULKAN_SRC_DIR}/vkDescriptorBuffer.cpp
${VULKAN_SRC_DIR}/vkShaderModule.cpp
)

set(VULKAN_SCENES_DIR src_vulkan/scenes)

set(VULKAN_ENGINE_SCENES_SOURCE
		${VULKAN_SCENES_DIR}/vkDeferredShadingContext.cpp
		${VULKAN_SCENES_DIR}/vkDeferred_Descriptor_Impl.cpp
		${VULKAN_SCENES_DIR}/vkDeferred_Framebuffer_Impl.cpp
		${VULKAN_SCENES_DIR}/vkDeferred_Pipeline_Impl.cpp
		${VULKAN_SCENES_DIR}/vkDeferred_RenderCommands_Impl.cpp
)

source_group("Source Files/VulkanSources" FILES ${VULKAN_ENGINE_SRC})
list(APPEND SOURCE_FILES ${VULKAN_ENGINE_SRC})

source_group("Source Files/VulkanSources/Scenes" FILES ${VULKAN_ENGINE_SCENES_SOURCE})
list(APPEND SOURCE_FILES ${VULKAN_ENGINE_SCENES_SOURCE})

#------------------CORE ENGINE HEADERS----------------------

set(HEADER_SRC_DIR src_core/headers)

set(HEADER_FILES
${HEADER_SRC_DIR}/Application.h
${HEADER_SRC_DIR}/Camera.h
${HEADER_SRC_DIR}/Common.h
${HEADER_SRC_DIR}/Object.h
${HEADER_SRC_DIR}/Physics.h
${HEADER_SRC_DIR}/Timer.h
${HEADER_SRC_DIR}/ThreadPool.h
${HEADER_SRC_DIR}/ApplicationGlobal.h
${HEADER_SRC_DIR}/CameraController.h
${HEADER_SRC_DIR}/SpirvHelper.h
${HEADER_SRC_DIR}/UserInterface.h
${HEADER_SRC_DIR}/Resource.h
${HEADER_SRC_DIR}/ResourceHandle.h
)

set(CORE_MANAGERS_HEADERS
		${CORE_MANAGERS_DIR}/SceneManager.h
		${CORE_MANAGERS_DIR}/ResourceManager.h
)

list(APPEND HEADER_FILES ${CORE_MANAGERS_HEADERS})

set(CORE_RESOURCES_HEADERS
	${CORE_RESOURCES_DIR}/GLTFModel.h
	${CORE_RESOURCES_DIR}/OBJModel.h
	${CORE_RESOURCES_DIR}/Model.h
	${CORE_RESOURCES_DIR}/primitives/Mesh.h
	${CORE_RESOURCES_DIR}/primitives/Vertex.h
)

list(APPEND HEADER_FILES ${CORE_RESOURCES_HEADERS})

#------------------VULKAN IMPLEMENTATION HEADERS----------------------
set(VULKAN_HEADER_DIR src_vulkan/headers)

set(VULKAN_HEADERS
${VULKAN_HEADER_DIR}/vkUtility.h
${VULKAN_HEADER_DIR}/vkGlobal.h
${VULKAN_HEADER_DIR}/vkInit.h
${VULKAN_HEADER_DIR}/vkWindow.h
${VULKAN_HEADER_DIR}/vkContextBase.h
${VULKAN_HEADER_DIR}/vkBuffer.h
${VULKAN_HEADER_DIR}/vkTexture.h
${VULKAN_HEADER_DIR}/vkSwapChain.h
${VULKAN_HEADER_DIR}/vkDevice.h
${VULKAN_HEADER_DIR}/vkDevice.h
${VULKAN_HEADER_DIR}/vkPipelineManager.h
${VULKAN_HEADER_DIR}/vkFramebuffer.h
${VULKAN_HEADER_DIR}/vkCubemap.h
${VULKAN_HEADER_DIR}/vkInstance.h
${VULKAN_HEADER_DIR}/vkDescriptorBuffer.h
${VULKAN_HEADER_DIR}/vkShaderModule.h
)

# vulkan scenes
set(VULKAN_SCENES_HEADERS ${VULKAN_SCENES_DIR}/vkDeferredShadingContext.h)

list(APPEND VULKAN_HEADERS ${VULKAN_SCENES_HEADERS})

source_group("Header Files/VulkanHeaders" FILES ${VULKAN_HEADERS})
list(APPEND HEADER_FILES ${VULKAN_HEADERS})

#------------------SHADERS----------------------
set(SHADER_SRC_DIR shaders)

set(SHADER_FILES
#deferred shading
${SHADER_SRC_DIR}/deferred-render/deMRT.vert
${SHADER_SRC_DIR}/deferred-render/deMRT.frag
${SHADER_SRC_DIR}/deferred-render/deComposition.vert
${SHADER_SRC_DIR}/deferred-render/deComposition.frag
#deferred shadowing
${SHADER_SRC_DIR}/deferred-render/deShadow.geom
${SHADER_SRC_DIR}/deferred-render/deShadow.vert
)


#-------------------------------------------MAKE THE PROJECT--------------------------------------
add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${SHADER_FILES} ${HEADER_FILES})

target_precompile_headers(${PROJECT_NAME} PUBLIC ${HEADER_SRC_DIR}/pch.h)

target_include_directories(${PROJECT_NAME} PRIVATE
src_core/headers
src_core/managers
src_core/resources
src_core/resources/primitives
src_vulkan #for scenes/
src_vulkan/headers
extern/stb
extern/tinyobjloader
extern/imgui
extern/imgui/backends
extern/fastgltf/include
extern/reactphysics3d/include
extern/shaderc/libshaderc/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
imgui #created this library within this script
SDL2::SDL2main
SDL2::SDL2-static
glm::glm
reactphysics3d
shaderc_combined
fastgltf::fastgltf
Vulkan::Vulkan
)




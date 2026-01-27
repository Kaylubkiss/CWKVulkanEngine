cmake_minimum_required(VERSION 3.22)

set(PROJECT_NAME "CKVulkan")
project("CKVulkan")

set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED)

#---------BUILD EXTERNAL LIBRARIES--------------
add_subdirectory(extern/fastgltf)
add_subdirectory(extern/reactphysics3d)

set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
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


#------------------SOURCES----------------------
set(SOURCE_FILES 
src/Application.cpp 
src/Camera.cpp
src/Common.cpp
src/Main.cpp
src/Object.cpp
src/Physics.cpp
src/Timer.cpp
src/ThreadPool.cpp
src/TextureManager.cpp
src/ObjectManager.cpp
src/ApplicationGlobal.cpp
src/CameraController.cpp
src/UserInterface.cpp
src/GLTFModel.cpp
src/OBJModel.cpp
)

set(VULKAN_SRC_DIR src/vulkan-srcs)

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
)

set(VULKAN_SCENES_DIR src/vulkan-scenes)

set(VULKAN_ENGINE_SCENES_SOURCE
		${VULKAN_SCENES_DIR}/vkDeferredShadingContext.cpp
		${VULKAN_SCENES_DIR}/vkDeferredShadingContext.h
)

source_group("Source Files/VulkanSources" FILES ${VULKAN_ENGINE_SRC})
list(APPEND SOURCE_FILES ${VULKAN_ENGINE_SRC})

source_group("Source Files/VulkanSources/Scenes" FILES ${VULKAN_ENGINE_SCENES_SOURCE})
list(APPEND SOURCE_FILES ${VULKAN_ENGINE_SCENES_SOURCE})

#------------------HEADERS----------------------

set(HEADER_SRC_DIR src/headers)

set(HEADER_FILES 
${HEADER_SRC_DIR}/Application.h
${HEADER_SRC_DIR}/Camera.h
${HEADER_SRC_DIR}/Common.h
${HEADER_SRC_DIR}/Object.h
${HEADER_SRC_DIR}/Vertex.h
${HEADER_SRC_DIR}/Physics.h
${HEADER_SRC_DIR}/Timer.h
${HEADER_SRC_DIR}/ThreadPool.h
${HEADER_SRC_DIR}/ObjectManager.h
${HEADER_SRC_DIR}/TextureManager.h
${HEADER_SRC_DIR}/ApplicationGlobal.h
${HEADER_SRC_DIR}/CameraController.h
${HEADER_SRC_DIR}/SpirvHelper.h
${HEADER_SRC_DIR}/UserInterface.h
${HEADER_SRC_DIR}/GLTFModel.h
${HEADER_SRC_DIR}/OBJModel.h
${HEADER_SRC_DIR}/IModel.h
)
				 
set(VULKAN_HEADER_DIR src/vulkan-headers)

set(VULKAN_ENGINE_HEADERS 
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
)

source_group("Header Files/VulkanHeaders" FILES ${VULKAN_ENGINE_HEADERS})	
list(APPEND HEADER_FILES ${VULKAN_ENGINE_HEADERS})


set(SHADER_SRC_DIR src/shaders)

set(SHADER_FILES
#blinn-phong
${SHADER_SRC_DIR}/blinnForward.frag	
${SHADER_SRC_DIR}/blinnForward.vert
#shadow mapping
${SHADER_SRC_DIR}/offscreenShadowMap.frag
${SHADER_SRC_DIR}/offscreenShadowMap.vert
${SHADER_SRC_DIR}/sceneShadowMap.frag
${SHADER_SRC_DIR}/sceneShadowMap.vert
${SHADER_SRC_DIR}/debugShadowMap.vert
${SHADER_SRC_DIR}/debugShadowMap.frag
#deferred shading
${SHADER_SRC_DIR}/deferredMRT.vert
${SHADER_SRC_DIR}/deferredMRT.frag
${SHADER_SRC_DIR}/deferredLightPass.vert
${SHADER_SRC_DIR}/deferredLightPass.frag
#deferred shadowing
${SHADER_SRC_DIR}/deferredShadow.geom
${SHADER_SRC_DIR}/deferredShadow.vert
)


#-------------------------------------------MAKE THE PROJECT--------------------------------------
add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${SHADER_FILES} ${HEADER_FILES})

target_precompile_headers(${PROJECT_NAME} PUBLIC ${HEADER_SRC_DIR}/pch.h)


target_include_directories(${PROJECT_NAME} PRIVATE 
src/headers
src/vulkan-headers
extern/stb
extern/tinyobjloader
extern/imgui
extern/imgui/backends
extern/fastgltf/include
extern/reactphysics3d/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
SDL2-static
SDL2main
reactphysics3d
shaderc_combined
imgui
fastgltf
Vulkan::Vulkan
)




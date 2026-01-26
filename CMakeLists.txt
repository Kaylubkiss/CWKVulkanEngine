cmake_minimum_required(VERSION 3.22)

set(PROJECT_NAME "CKVulkan")

#vcpkg will install fastgltf
if (WIN32)
	set(CMAKE_TOOLCHAIN_FILE C:/vcpkg/scripts/buildsystems/vcpkg.cmake)
endif()

project(${PROJECT_NAME} LANGUAGES CXX C)

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
${VULKAN_SCENES_DIR}/vkDeferredShadingContext.cpp ${VULKAN_SCENES_DIR}/vkDeferredShadingContext.h
)

source_group("Source Files/VulkanSources" FILES ${VULKAN_ENGINE_SRC})
list(APPEND SOURCE_FILES ${VULKAN_ENGINE_SRC})

source_group("Source Files/VulkanSources/Scenes" FILES ${VULKAN_ENGINE_SCENES_SOURCE})
list(APPEND SOURCE_FILES ${VULKAN_ENGINE_SCENES_SOURCE})	

set(IM_GUI_FILES
src/external/libraries/imgui/imgui_demo.cpp
src/external/libraries/imgui/imgui_draw.cpp
src/external/libraries/imgui/imgui_tables.cpp
src/external/libraries/imgui/imgui_widgets.cpp
src/external/libraries/imgui/imgui.cpp
src/external/libraries/imgui/backends/imgui_impl_sdl2.cpp
src/external/libraries/imgui/backends/imgui_impl_vulkan.cpp
)

source_group("Source Files/ImGui" FILES ${IM_GUI_FILES})			
list(APPEND SOURCE_FILES ${IM_GUI_FILES})					 

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
								 
#-----COMPILATION OPTIONS DEPENDING ON PLATFORM---------
#if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
   # string(APPEND CMAKE_CXX_FLAGS "/MP")
#	message("Using MSVC compiler with C++")
#elseif(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
   # string(APPEND CMAKE_C_FLAGS " /MP")
	#message("Using MSVC compiler with C")
#endif()

#-------------------------------------------MAKE THE PROJECT--------------------------------------
add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${SHADER_FILES} ${HEADER_FILES})

target_precompile_headers(${PROJECT_NAME} PUBLIC ${HEADER_SRC_DIR}/pch.h)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

find_package(Vulkan REQUIRED)

find_package(fastgltf CONFIG REQUIRED)



if (UNIX)
	find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)

	# 1. Look for a SDL2 package, 2. Look for the SDL2maincomponent and 3. DO NOT fail when SDL2main is not available
	find_package(SDL2 REQUIRED CONFIG COMPONENTS SDL2main)
endif()


target_include_directories(${PROJECT_NAME} PRIVATE 
src/headers
src/vulkan-headers
src/external/libraries
src/external/libraries/imgui
src/external/libraries/stb
src/external/libraries/ReactPhysics3D/include
)

if (WIN32)
	target_include_directories(${PROJECT_NAME} PRIVATE $ENV{VULKAN_SDK}/Include)

	target_link_libraries("${PROJECT_NAME}" 
	SDL2.lib
	vulkan-1.lib
	SDL2main.lib
	fastgltf::fastgltf)

	target_link_libraries("${PROJECT_NAME}" )

	target_link_directories("${PROJECT_NAME}" PUBLIC $ENV{VULKAN_SDK}/Lib 
	src/External/libraries/ReactPhysics3D/lib)
	
	target_link_libraries(${PROJECT_NAME} debug reactphysics3dd.lib general reactphysics3d.lib)
	target_link_libraries(${PROJECT_NAME} debug shaderc_combinedd.lib general shaderc_combined.lib)
	target_link_options(${PROJECT_NAME} PRIVATE "/ignore:4099") #reactphysics, SDL2, and shaderc_combined are missing their debug information. Just ignore it.
elseif (UNIX)
	if(TARGET SDL2::SDL2main)
		# It has an implicit dependency on SDL2 functions, so it MUST be added before SDL2::SDL2 (or SDL2::SDL2-static)
		target_link_libraries(${PROJECT_NAME} SDL2::SDL2main)
	endif()

	set(GIT_LIBRARIES $ENV{HOME}/vkEngineLibraries)
	target_include_directories(${PROJECT_NAME} PRIVATE 
		${GIT_LIBRARIES}/shaderc/libshaderc/include
		${GIT_LIBRARIES}/reactphysics3d/include/reactphysics3d)
	
	target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARIES} SDL2::SDL2 reactphysics3d libshaderc_combined.a)
	
	target_link_directories(${PROJECT_NAME} PRIVATE 
		Vulkan::Vulkan
		${GIT_LIBRARIES}/shaderc/build/libshaderc
		${GIT_LIBRARIES}/reactphysics3d/build/
	)

	target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=address)
	target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address)
endif()




 cmake_minimum_required(VERSION 3.22)

set(PROJECT_NAME "CKVulkan")

project("${PROJECT_NAME}" LANGUAGES CXX C)

#------------------SOURCES----------------------
set(SOURCE_FILES src/Application.cpp 
                   src/BlinnPhong.cpp 
                   src/Camera.cpp 
                   src/Common.cpp
                   src/Debug.cpp
                   src/Main.cpp
                   src/Object.cpp
                   src/Physics.cpp
                   src/Timer.cpp
                   src/ThreadPool.cpp
				   src/TextureManager.cpp
				   src/ObjectManager.cpp
				   src/ApplicationGlobal.cpp
				   src/CameraController.cpp
				   src/HotReloader.cpp
				   src/UserInterface.cpp
)
							
set(VULKAN_ENGINE_SRC src/vkDebug.cpp
					  src/vkUtility.cpp
					  src/vkInit.cpp
                      src/vkGlobal.cpp
                      src/vkContextBase.cpp
					  src/vkSwapChain.cpp
					  src/vkBuffer.cpp
					  src/vkTexture.cpp
					  src/vkPipeline.cpp
					  src/vkDevice.cpp
					  src/vkMesh.cpp
					  src/vkWindow.cpp
)

set(VULKAN_ENGINE_SCENES_SOURCE 
src/vkFreddyHeadContext.cpp
src/vkDeferredShadingContext.cpp
src/vkShadowMapContext.cpp)




source_group("Source Files/VulkanSources" FILES ${VULKAN_ENGINE_SRC})			
list(APPEND SOURCE_FILES ${VULKAN_ENGINE_SRC})

source_group("Source Files/VulkanSources/Scenes" FILES ${VULKAN_ENGINE_SCENES_SOURCE})
list(APPEND SOURCE_FILES ${VULKAN_ENGINE_SCENES_SOURCE})	

set(IM_GUI_FILES src/External/libraries/imgui/imgui_demo.cpp
                   src/External/libraries/imgui/imgui_draw.cpp
                   src/External/libraries/imgui/imgui_tables.cpp
                   src/External/libraries/imgui/imgui_widgets.cpp
                   src/External/libraries/imgui/imgui.cpp
                   src/External/libraries/imgui/backends/imgui_impl_sdl2.cpp
                   src/External/libraries/imgui/backends/imgui_impl_vulkan.cpp)

if (WIN32)
	#currently doesn't work..
	list(APPEND ${IM_GUI_FILES} src/External/libraries/imgui/backends/imgui_impl_win32.cpp)
endif()

source_group("Source Files/ImGui" FILES ${IM_GUI_FILES})			
list(APPEND SOURCE_FILES ${IM_GUI_FILES})					 

#------------------HEADERS----------------------
set(HEADER_FILES 
src/Include/Application.h
src/Include/BlinnPhong.h
src/Include/Camera.h
src/Include/Common.h
src/Include/Debug.h
src/Include/Object.h
src/Include/Vertex.h
src/Include/Physics.h
src/Include/Timer.h
src/Include/ThreadPool.h
src/Include/ObjectManager.h
src/Include/TextureManager.h
src/Include/ApplicationGlobal.h
src/Include/Controller.h
src/Include/SpirvHelper.h
src/Include/HotReloader.h
src/Include/UserInterface.h
)
				 

set(VULKAN_ENGINE_HEADERS 
src/Include/vkUtility.h
src/Include/vkDebug.h
src/Include/vkGlobal.h
src/Include/vkInit.h
src/Include/vkWindow.h
src/Include/vkContextBase.h
src/Include/vkBuffer.h
src/Include/vkTexture.h
src/Include/vkSwapChain.h
src/Include/vkMesh.h
src/Include/vkPipeline.h
src/Include/vkDevice.h
src/Include/vkShadowMap.h
src/Include/vkDevice.h
)


set(VULKAN_ENGINE_SCENES_HEADERS
src/Include/vkFreddyHeadContext.h
src/Include/vkDeferredShadingContext.h
src/Include/vkShadowMapContext.h
)

source_group("Header Files/VulkanHeaders" FILES ${VULKAN_ENGINE_HEADERS})	
list(APPEND HEADER_FILES ${VULKAN_ENGINE_HEADERS})

source_group("Header Files/VulkanHeaders/Scenes" FILES ${VULKAN_ENGINE_SCENES_HEADERS})
list(APPEND HEADER_FILES ${VULKAN_ENGINE_SCENES_HEADERS})	

set(SHADER_FILES
#blinn-phong
src/Shaders/blinnForward.frag	
src/Shaders/blinnForward.vert
#shadow mapping
src/Shaders/offscreenShadowMap.frag
src/Shaders/offscreenShadowMap.vert
src/Shaders/sceneShadowMap.frag
src/Shaders/sceneShadowMap.vert
src/Shaders/debugShadowMap.vert
src/Shaders/debugShadowMap.frag
#deferred shading
src/Shaders/deferredMRT.vert
src/Shaders/deferredMRT.frag
src/Shaders/deferredLightPass.vert
src/Shaders/deferredLightPass.frag
)
								 
#-----COMPILATION OPTIONS DEPENDING ON PLATFORM---------
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    string(APPEND CMAKE_CXX_FLAGS "/MP")
		message("Using MSVC compiler with C++")
elseif(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    string(APPEND CMAKE_C_FLAGS " /MP")
		message("Using MSVC compiler with C")
endif()

#-------------------------------------------MAKE THE PROJECT--------------------------------------
add_executable("${PROJECT_NAME}" ${SOURCE_FILES} ${SHADER_FILES} ${HEADER_FILES})


target_precompile_headers("${PROJECT_NAME}" PUBLIC "src/Include/pch.h")

set_property(TARGET "${PROJECT_NAME}" PROPERTY CXX_STANDARD 20)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT "${PROJECT_NAME}")

find_package(Vulkan REQUIRED)

if (UNIX)

	find_package(ReactPhysics3D REQUIRED)

	find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)

	# 1. Look for a SDL2 package, 2. Look for the SDL2maincomponent and 3. DO NOT fail when SDL2main is not available
	find_package(SDL2 REQUIRED CONFIG COMPONENTS SDL2main)

endif()


target_include_directories("${PROJECT_NAME}" PRIVATE src/Include
src/External/libraries
src/External/libraries/imgui
src/External/libraries/stb
src/External/libraries/ReactPhysics3D/include)

if (WIN32)
	target_include_directories(${PROJECT_NAME} PRIVATE $ENV{VULKAN_SDK}/Include)

	target_link_libraries("${PROJECT_NAME}" SDL2.lib
	vulkan-1.lib
	SDL2main.lib)

	target_link_libraries(${PROJECT_NAME} debug reactphysics3dd.lib general reactphysics3d.lib)

	target_link_libraries(${PROJECT_NAME} debug shaderc_combinedd.lib)

	target_link_directories("${PROJECT_NAME}" PRIVATE $ENV{VULKAN_SDK}/Lib src/External/libraries/ReactPhysics3D/lib)

elseif (UNIX)

	if(TARGET SDL2::SDL2main)
		# It has an implicit dependency on SDL2 functions, so it MUST be added before SDL2::SDL2 (or SDL2::SDL2-static)
		target_link_libraries(${PROJECT_NAME} SDL2::SDL2main)
	endif()

	target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARIES} SDL2::SDL2 reactphysics3d shaderc_combined)

	#~/libraries contains reactphysics3d and shaderc_combined
	target_link_directories("${PROJECT_NAME}" PRIVATE Vulkan::Vulkan ~/libraries/)

	target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=address)
	target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address)
endif()




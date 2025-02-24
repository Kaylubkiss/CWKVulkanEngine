cmake_minimum_required(VERSION 3.22)

set(PROJECT_NAME "CKVulkan")

project("${PROJECT_NAME}" LANGUAGES CXX C)


set(VULKAN_ENGINE_SRC src/vkDebug.cpp
											src/vkUtility.cpp
											src/vkInit.cpp
                      src/vkGlobal.cpp
                      src/vkGraphicsSystem.cpp
											src/vkSwapChain.cpp
											src/vkBuffer.cpp
											src/vkResource.cpp
											src/vkRenderResources.cpp)


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
                   src/External/libraries/imgui/imgui_demo.cpp
                   src/External/libraries/imgui/imgui_draw.cpp
                   src/External/libraries/imgui/imgui_tables.cpp
                   src/External/libraries/imgui/imgui_widgets.cpp
                   src/External/libraries/imgui/imgui.cpp
                   src/External/libraries/imgui/backends/imgui_impl_sdl2.cpp
                   src/External/libraries/imgui/backends/imgui_impl_vulkan.cpp
                   src/External/libraries/imgui/backends/imgui_impl_win32.cpp
                   ${VULKAN_ENGINE_SRC})
									 
									 
#set(SRC_INC src/Include)
set(HEADER_FILES src/Include/Application.h
								 src/Include/BlinnPhong.h
				         src/Include/Camera.h
								 src/Include/Common.h
								 src/Include/Debug.h
								 src/Include/Object.h
								 src/Include/Physics.h
								 src/Include/Timer.h
								 src/Include/ThreadPool.h
								 src/Include/ObjectManager.h
								 src/Include/TextureManager.h
								 src/Include/ApplicationGlobal.h
								 src/Include/Controller.h
								 src/Include/vkUtility.h
								 src/Include/vkDebug.h
								 src/Include/vkGlobal.h
								 src/Include/vkInit.h
								 src/Include/vkWindow.h
								 src/Include/vkGraphicsSystem.h
								 src/Include/vkBuffer.h
								 src/Include/vkSwapChain.h
								 src/Include/vkResource.h
								 src/Include/vkMesh.h
								 src/Include/vkRenderResources.h)												
												
									 
set(SHADER_FILES src/Shaders/blinn.frag
								 src/Shaders/blinn.vert)	 




add_executable("${PROJECT_NAME}"  ${SOURCE_FILES} ${SHADER_FILES} ${HEADER_FILES})
	
source_group("Objects" FILES Objects.cpp)

target_precompile_headers("${PROJECT_NAME}" PUBLIC "src/Include/pch.h")

set_property(TARGET "${PROJECT_NAME}" PROPERTY CXX_STANDARD 20)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    string(APPEND CMAKE_CXX_FLAGS " /MP")
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:library")
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    string(APPEND CMAKE_C_FLAGS " /MP")
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:library")
endif()




set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT "${PROJECT_NAME}")


find_package(Vulkan REQUIRED)

target_include_directories("${PROJECT_NAME}" PRIVATE src/Include
                                       src/External/libraries
                                       src/External/libraries/imgui
                                       src/External/libraries/stb
                                       src/External/libraries/ReactPhysics3D/include
                                        $ENV{VULKAN_SDK}/Include)


target_link_libraries("${PROJECT_NAME}" SDL2.lib
                                        vulkan-1.lib
										SDL2main.lib
										reactphysics3d.lib)



target_link_directories("${PROJECT_NAME}" PRIVATE $ENV{VULKAN_SDK}/Lib src/External/libraries/ReactPhysics3D/lib)
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
											src/vkResource.cpp)


set(SOURCE_FILES src/Application.cpp 
                   src/BlinnPhong.cpp 
                   src/Camera.cpp 
                   src/Common.cpp
                   src/Controller.cpp
                   src/Debug.cpp
                   src/Main.cpp
                   src/Object.cpp
                   src/Physics.cpp
                   src/Timer.cpp
                   src/ThreadPool.cpp
									 src/TextureManager.cpp
									 src/ObjectManager.cpp
                   src/External/libraries/imgui/imgui_demo.cpp
                   src/External/libraries/imgui/imgui_draw.cpp
                   src/External/libraries/imgui/imgui_tables.cpp
                   src/External/libraries/imgui/imgui_widgets.cpp
                   src/External/libraries/imgui/imgui.cpp
                   src/External/libraries/imgui/backends/imgui_impl_sdl2.cpp
                   src/External/libraries/imgui/backends/imgui_impl_vulkan.cpp
                   src/External/libraries/imgui/backends/imgui_impl_win32.cpp
                   ${VULKAN_ENGINE_SRC})
									 
									 
set(SRC_INC src/Include)
set(HEADER_FILES ${SRC_INC}/Application.h
				 ${SRC_INC}/BlinnPhong.h
				 ${SRC_INC}/Camera.h
				 ${SRC_INC}/Common.h
				 ${SRC_INC}/Controller.h
				 ${SRC_INC}/Debug.h
				 ${SRC_INC}/Object.h
				 ${SRC_INC}/Physics.h
				 ${SRC_INC}/Timer.h
				 ${SRC_INC}/ThreadPool.h
				 ${SRC_INC}/ObjectManager.h
				 ${SRC_INC}/TextureManager.h
				 ${SRC_INC}/vkUtility.h
				 ${SRC_INC}/vkDebug.h
				 ${SRC_INC}/vkGlobal.h
				 ${SRC_INC}/vkInit.h
         ${SRC_INC}/vkWindow.h
         ${SRC_INC}/vkGraphicsSystem.h
				 ${SRC_INC}/vkBuffer.h
				 ${SRC_INC}/vkSwapChain.h
				 ${SRC_INC}/vkResource.h)												
												
									 
set(SHADER_DIR src/Shaders)
set(SHADER_FILES ${SHADER_DIR}/blinn.frag
				 ${SHADER_DIR}/blinn.vert)	 

add_executable("${PROJECT_NAME}"  ${SOURCE_FILES}  ${SHADER_FILES} ${HEADER_FILES})
	


target_precompile_headers("${PROJECT_NAME}" PUBLIC "src/Include/pch.h")

set_property(TARGET "${PROJECT_NAME}" PROPERTY CXX_STANDARD 20)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    string(APPEND CMAKE_CXX_FLAGS " /MP")
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    string(APPEND CMAKE_C_FLAGS " /MP")
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
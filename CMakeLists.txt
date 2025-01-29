cmake_minimum_required(VERSION 3.22)

set(PROJECT_NAME "CKVulkan")

project("${PROJECT_NAME}" LANGUAGES CXX C)

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
                   src/External/libraries/imgui/imgui_demo.cpp
                   src/External/libraries/imgui/imgui_draw.cpp
                   src/External/libraries/imgui/imgui_tables.cpp
                   src/External/libraries/imgui/imgui_widgets.cpp
                   src/External/libraries/imgui/imgui.cpp
                   src/External/libraries/imgui/backends/imgui_impl_sdl2.cpp
                   src/External/libraries/imgui/backends/imgui_impl_vulkan.cpp
                   src/External/libraries/imgui/backends/imgui_impl_win32.cpp)
									 
									 
set(SRC_INC src/Include)
file(GLOB HEADER_FILES ${SRC_INC}/Application.h
												${SRC_INC}/BlinnPhong.h
												${SRC_INC}/Camera.h
												${SRC_INC}/Common.h
												${SRC_INC}/Controller.h
												${SRC_INC}/Debug.h
												${SRC_INC}/Object.h
												${SRC_INC}/Physics.h
												${SRC_INC}/Timer.h)												
																				
add_executable("${PROJECT_NAME}"  src/Application.cpp 
                   src/BlinnPhong.cpp 
                   src/Camera.cpp 
                   src/Common.cpp
                   src/Controller.cpp
                   src/Debug.cpp
                   src/Main.cpp
                   src/Object.cpp
                   src/Physics.cpp
                   src/Timer.cpp
                   src/External/libraries/imgui/imgui_demo.cpp
                   src/External/libraries/imgui/imgui_draw.cpp
                   src/External/libraries/imgui/imgui_tables.cpp
                   src/External/libraries/imgui/imgui_widgets.cpp
                   src/External/libraries/imgui/imgui.cpp
                   src/External/libraries/imgui/backends/imgui_impl_sdl2.cpp
                   src/External/libraries/imgui/backends/imgui_impl_vulkan.cpp
                   src/External/libraries/imgui/backends/imgui_impl_win32.cpp ${HEADER_FILES})

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
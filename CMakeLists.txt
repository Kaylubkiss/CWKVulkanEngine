cmake_minimum_required(VERSION 3.22)

set(PROJECT_NAME "CWKVULKAN")

project("${PROJECT_NAME}" LANGUAGES CXX C)

add_executable("${PROJECT_NAME}" FrameWork/Application.cpp 
                   FrameWork/BlinnPhong.cpp 
                   FrameWork/Camera.cpp 
                   FrameWork/Common.cpp
                   FrameWork/Controller.cpp
                   FrameWork/Debug.cpp
                   FrameWork/Main.cpp
                   FrameWork/Object.cpp
                   FrameWork/Physics.cpp
                   FrameWork/Timer.cpp
                   FrameWork/External/libraries/imgui/imgui_demo.cpp
                   FrameWork/External/libraries/imgui/imgui_draw.cpp
                   FrameWork/External/libraries/imgui/imgui_tables.cpp
                   FrameWork/External/libraries/imgui/imgui_widgets.cpp
                   FrameWork/External/libraries/imgui/imgui.cpp
                   FrameWork/External/libraries/imgui/backends/imgui_impl_sdl2.cpp
                   FrameWork/External/libraries/imgui/backends/imgui_impl_vulkan.cpp
                   FrameWork/External/libraries/imgui/backends/imgui_impl_win32.cpp)

target_precompile_headers("${PROJECT_NAME}" PUBLIC FrameWork/pch.h)

set_property(TARGET "${PROJECT_NAME}" PROPERTY CXX_STANDARD 20)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    string(APPEND CMAKE_CXX_FLAGS " /MP")
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    string(APPEND CMAKE_C_FLAGS " /MP")
endif()

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT "${PROJECT_NAME}")


find_package(Vulkan REQUIRED)

target_include_directories("${PROJECT_NAME}" PRIVATE FrameWork
                                       FrameWork/External/libraries
                                       FrameWork/External/libraries/imgui/
                                       FrameWork/External/libraries/stb/
                                       FrameWork/External/libraries/ReactPhysics3D/include
                                       $ENV{VULKAN_SDK}/Include)


target_link_libraries("${PROJECT_NAME}" SDL2.lib
                          vulkan-1.lib
                          SDL2main.lib
                          reactphysics3d.lib)



target_link_directories("${PROJECT_NAME}" PRIVATE $ENV{VULKAN_SDK}/Lib FrameWork/External/libraries/ReactPhysics3D/lib)



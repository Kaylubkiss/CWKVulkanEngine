cmake_minimum_required(VERSION 3.22)

set(PROJECT_NAME "CKVulkan")

project("${PROJECT_NAME}" LANGUAGES CXX C)

file(GLOB_RECURSE CPP_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src" *.cpp)
add_executable("${PROJECT_NAME}" ${CPP_FILES})

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



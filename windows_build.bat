@echo off

::make sure that vulkan is installed.
winget install --id=KhronosGroup.VulkanSDK  -e

::make sure cmake is installed.
winget install --id=Kitware.CMake -e

::make sure to have ninja for build generation
winget install --id=Ninja-build.Ninja -e

set previous_directory=%cd%

cd %VULKAN_SDK%

maintenancetool.exe --accept-licenses --default-answer --confirm-command install ^
com.lunarg.vulkan.volk ^
com.lunarg.vulkan.vma ^
com.lunarg.vulkan.debug

cd %previous_directory%

mkdir build

cd build

cmake ..

ninja

pause
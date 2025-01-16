::@echo off

curl https://sdk.lunarg.com/sdk/download/1.4.304.0/windows/VulkanSDK-1.4.304.0-Installer.exe --output VulkanInstaller.exe


VulkanInstaller.exe --accept-licenses --default-answer --root "%~dp0\VulkanSDK" --confirm-command install com.lunarg.vulkan.32bit com.lunarg.vulkan.sdl2 com.lunarg.vulkan.glm com.lunarg.vulkan.vma com.lunarg.vulkan.debug com.lunarg.vulkan.debug32

cd /d "%~dp0" 

mkdir build

cd build

cmake ..

PAUSE
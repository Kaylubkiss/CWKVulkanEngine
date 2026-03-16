# Caleb's Custom Vulkan Graphics Engine

[![Windows Build](https://github.com/Kaylubkiss/CWKVulkanEngine/actions/workflows/windows-build.yml/badge.svg?branch=dev)](https://github.com/Kaylubkiss/CWKVulkanEngine/actions/workflows/windows-build.yml)
[![Linux Build](https://github.com/Kaylubkiss/CWKVulkanEngine/actions/workflows/linux-build.yml/badge.svg?branch=dev)](https://github.com/Kaylubkiss/CWKVulkanEngine/actions/workflows/linux-build.yml)

Hi, welcome to my Github page! As of September 25th, 2025, I've made this repository public for all to see my progress! There'll be lots of updates in the coming weeks, months, maybe even years!

<div>
  <img width="479" height="363" alt="basic shadow maping" src="https://github.com/user-attachments/assets/e817a626-740a-49e9-a7aa-8942fc1c1205" />
  <p><em>A shadow mapping implementation written under my codebase</em></p>
</div>

# Goals for this project

* Learn Vulkan and GPU architecture
* Implement algorithms and build scalable systems for graphics development
* Strengthen my tooling mindset
* Have fun and make some cool freakin demos to share

<p>My dream has always been to become an elite toy maker whose creations delight users around the world.</p> 
<p>This project aims to fulfill that.</p>

# (3) Highlighted Features

## Hot Reloading

![Hot Reloading Video](https://github.com/user-attachments/assets/79b6e14c-a74a-4d81-863c-fd1512a32f3a)

## Async Asset Loading

![Async Loading Video](https://github.com/user-attachments/assets/6d235866-6575-496f-8ad8-f589ba934301)

## PBR Texture Rendering

[ TODO - show video of singular object in space ]



# Build Instructions 
**General Requirements:**

<ul>
     <li>C++20 compatible compiler/linker/debugger
        <ul>
            <li>Bundled with Microsoft Visual Studio (Desktop C++ Component)</li>
            <li>gcc, clang/LLVM specifically for other platforms</li>
        </ul>
    </li>
    <li><a href="https://vulkan.lunarg.com/sdk/home">Vulkan SDK (1.4+)</a></li>
    <li>
        <a href="https://cmake.org/download/">CMake versions 3.2-3.5</a>
        <ul>
            <li>Bundled with Visual Studio (C++ CMake Tools for Windows Component)</li>
        </ul>
    </li>
    <li>Ninja build generator (required to run .bat/.sh scripts)
        <ul>
            <li>Bundled with Visual Studio CMake tools</li>
            <li>Can use another build generator as well but you'll have to build manually</li>
        </ul>
    </li>
    <li><a href="https://www.python.org/downloads/">Python 3.13+</a> (required in order to compile shaderc)</li>
   
    
</ul>

<p>Ensure to have the correct drivers installed for Vulkan 1.4 and above according to your hardware vendor:</p>
<ul>
  <li><a href="https://www.amd.com/en/support/download/drivers.html">AMD</a></li>
  <li><a href="https://www.nvidia.com/en-us/drivers/">Nvidia</a></li>
  <li><a href="https://www.intel.com/content/www/us/en/download-center/home.html">Intel</a></li>
  <li>other...</li>
</ul> 

An IDE is <em>highly</em> recommended for development:
<ul>
    <li><a href="https://visualstudio.microsoft.com/downloads/">Visual Studio (for Windows)</a></li>
    <li><a href="https://www.jetbrains.com/clion/">CLion (cross-platform)</a></li>
    <li>...</li>
    
</ul>

**Windows:**

  <ol>
    <li><a href="https://visualstudio.microsoft.com/downloads/">
    Download and install the latest version of Visual Studio</a>
        <ol>
            <li>Check <b><em>Desktop development with C++</em></b> under the workloads tab in the installer</li>
            <li>Ensure you have MSVC build tools v143 or higher (Check <b><em>Optional</em></b> dropdown)</li>
        </ol>
    </li>
    <li>Clone the repo</li>
    <li>Run the <b><em>batch (.bat)</em></b> script</li>
    <li>Go to <b><em>build</em></b> folder</li>
    <li>Open .sln, run the project, mess with it, etc.</li>
    <li>Start Coding</li>
  </ol> 

**Linux:** 

<em><b>NOTE:</b> This was tested on WSL, Ubuntu 25.04 (Plucky Puffin), and Ubuntu 24.04 (Noble Numbat).</em>

<ol>
  <li>Clone the repo</li>
  <li>Run the <em>bash (.sh)</em> script</li>
  <li>Go to build folder, run the produced executable ("CKVulkan"), mess with it, etc.</li>
  <li>Start Coding</li>
</ol> 

# Works In Progress (Updated 01/31/2026)
<ul>
  <li>Animation and Material support through <em><a href="https://www.khronos.org/Gltf">.glTF</a></em> specifications and GLSL shading</li>
</ul>

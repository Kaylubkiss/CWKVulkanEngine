#!/bin/bash

sudo apt-get update && sudo apt-get upgrade

sudo apt-get install libvulkan-dev \
libvulkan1 \
vulkan-tools \
vulkan-validationlayers \
libsdl2-dev \
libglm-dev \
ninja-build \
cmake \
build-essential

current_directory=$(pwd)

cd ~/

library_directory=vkEngineLibraries/

mkdir $library_directory

cd $library_directory

#BEGIN download shaderc library
git clone https://github.com/google/shaderc

cd shaderc

./utils/git-sync-deps

mkdir build

cd build

cmake -G Ninja ..

ninja -j 4

cd ..

#END download shaderc library

#BEGIN download reactphysics3d
git clone https://github.com/DanielChappuis/reactphysics3d.git reactphysics3d

cd reactphysics3d

mkdir build

cd build

cmake -G Ninja ..

ninja -j 4
#END download reactphysic3d

cd $current_directory

mkdir build

cd build

cmake -G Ninja ..

ninja

read -p "Script over, press anything to continue." 

cd ..

#!/bin/bash

git submodule update --init --recursive

sudo apt-get install libvulkan-dev \
libvulkan1 \
vulkan-tools \
vulkan-validationlayers \
ninja-build \
cmake \
build-essential \
python3

python3 extern/shaderc/utils/git-sync-deps

mkdir build

cd build

cmake -G Ninja ..

ninja

read -p "Script over, press anything to continue." 

cd ..

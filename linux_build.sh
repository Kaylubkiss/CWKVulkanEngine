#!/bin/bash

sudo apt-get update && sudo apt-get upgrade

sudo apt-get install libvulkan-dev \
libvulkan1 \
vulkan-tools \
vulkan-validationlayers \
ninja-build \
cmake \
build-essential

mkdir build

cd build

cmake -G Ninja ..

ninja

read -p "Script over, press anything to continue." 

cd ..

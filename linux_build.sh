#!/bin/bash

sudo apt-get update && sudo apt-get upgrade

sudo apt-get install libvulkan-dev libvulkan1 vulkan-tools libsdl2-dev libglm-dev ninja

current_directory=$(pwd)

cd src/External/libraries

git clone https://github.com/google/shaderc

cd shaderc

./utils/git-sync-deps

cd $current_directory

rm -r build

mkdir build

cd build

cmake -G Ninja ..

ninja

read -p "Script over, press anything to continue." 

cd ..
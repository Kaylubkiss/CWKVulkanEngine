#!/bin/bash

mkdir build

cd build

cmake ..

echo "please enter a build generator command (e.g. Ninja)"

read user_command

$user_command

read -p "Script over, press anything to continue." 
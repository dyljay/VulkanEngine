#!/bin/bash

# stop on error
set -e

# compile shaders
#./compile_shaders.sh

# make if does not exist
mkdir -p build

# configure
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug

# symlink the compile_commands
ln -sf build/compile_commands.json compile_commands.json

# build engine
cmake --build build --parallel

# Run it
./build/VulkanEngine

#!/bin/bash

# stop on error
set -e

# setting build_type to first argument and "Debug" on default
BUILD_TYPE=${1:-Debug}

# determining whether the argument matches one of the possible arguments
case "$BUILD_TYPE" in Debug | Release | RelWithDebInfo) ;;
*)
  echo "usage: $0 [Debug|Release|RelWithDebInfo]"
  exit 1
  ;;
esac

# creating build folder name
BUILD_DIR=build-$(echo "$BUILD_TYPE" | tr '[:upper:]' '[:lower:]')

# make if does not exist
mkdir -p "$BUILD_DIR"

# configure
cmake -S . -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE="$BUILD_TYPE"

# symlink the compile_commands
ln -sf "$BUILD_DIR"/compile_commands.json compile_commands.json

# build engine
cmake --build "$BUILD_DIR" --parallel

# Run it
./"$BUILD_DIR"/VulkanEngine

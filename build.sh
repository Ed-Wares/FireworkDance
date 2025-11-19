#!/bin/bash
current_dir=$(dirname "$(realpath "$0")" )
build_dir="$current_dir/build"

echo creating build dir "$build_dir"
mkdir -p "$build_dir"
mkdir -p "$current_dir/distrib"
cd "$build_dir"

#REM echo Installing dependencies with vcpkg
#REM vcpkg install glfw3 glm freetype

echo Configuring and building with CMake
cmake ..
cmake --build .
cp -v *.exe "$current_dir/distrib"
cp -v *.dll "$current_dir/distrib"
cd ..

# rm -fr ./build
#!/bin/bash
current_dir=$(dirname "$(realpath "$0")" )
build_dir="$current_dir/build"

echo removing old build
rm -fr "$build_dir"

echo creating build directory "$build_dir"
mkdir -p "$build_dir"
mkdir -p "$current_dir/distrib"
cd "$build_dir"

#echo Install dependencies with cmd: vcpkg install glfw3 glm freetype
echo dependencies can be installed in MinGW with: pacman -S mingw-w64-ucrt-x86_64-glfw mingw-w64-ucrt-x86_64-glm mingw-w64-ucrt-x86_64-freetype

echo Configuring and building with CMake
cmake ..
cmake --build . --config Release

echo copying binaries to the distrib folder...
cp -v *.exe "$current_dir/distrib"
cp -v *.dll "$current_dir/distrib"
cd ..

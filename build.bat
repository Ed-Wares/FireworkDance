@echo off
SET "current_dir=%~dp0"
SET build_dir=%current_dir%build
SET msys2bash=c:\msys64\msys2_shell.cmd -ucrt64 -c 

cd /d "%current_dir%"

echo removing old build
rmdir /s /q "%build_dir%"

echo creating build directory "$build_dir"
mkdir %build_dir%
mkdir "%current_dir%\distrib"
cd %build_dir%


REM echo Install dependencies with cmd: vcpkg install glfw3 glm freetype
echo dependencies can be installed in MinGW with: pacman -S mingw-w64-ucrt-x86_64-glfw mingw-w64-ucrt-x86_64-glm mingw-w64-ucrt-x86_64-freetype

echo Configuring and building with CMake
cmake .. -G "MinGW Makefiles"
REM cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release

echo copying binaries to the distrib folder...
copy /Y *.exe "%current_dir%\distrib"
copy /Y *.dll "%current_dir%\distrib"

cd ..

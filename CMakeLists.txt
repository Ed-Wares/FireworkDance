cmake_minimum_required(VERSION 3.10)
project(FireworkDance)

# Require C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Dependency Finding and Linking ---

# Find GLFW (installed via pacman, e.g., 'mingw-w64-x86_64-glfw')
# This assumes CMake is run from an MSYS2 MinGW shell, which sets up the search paths correctly.
# Find GLFW and GLM
find_package(glfw3 CONFIG REQUIRED) # Use CONFIG for modern vcpkg GLFW
find_package(glm CONFIG REQUIRED)

# Find OpenGL (required by GLAD)
find_package(OpenGL REQUIRED)

# Add an include directory for GLAD's headers
include_directories(vendor/glad/include vendor/stb)

# --- Build Executable ---
add_executable(FireworkDance
    src/FireworkDance.cpp
    src/stb_impl.cpp
    vendor/glad/src/glad.c
)

# Link the executable against GLFW, OpenGL, and GLM
target_link_libraries(FireworkDance PRIVATE glfw)

if(WIN32)
    target_link_options(FireworkDance PRIVATE -mwindows )
endif()

# Ensure executable can find its DLLs on Windows if needed (useful for development)
if (WIN32)
    # This copies necessary DLLs (like glfw3.dll) from vcpkg to the build output directory
    file(GLOB DLL_FILES "$ENV{VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/bin/*.dll")
    install(FILES ${DLL_FILES} DESTINATION "$<TARGET_FILE_DIR:FireworkDance>")
endif()

# POST_BUILD DLL COPYING (Re-using the logic from the previous turn)
# This ensures runtime DLLs are copied to the build directory.
if(WIN32 AND CMAKE_C_COMPILER_ID MATCHES "GNU")
    message(STATUS "Setting up POST_BUILD command to copy MinGW runtime DLLs and GLFW DLL.")

    get_filename_component(MINGW_BIN_DIR "${CMAKE_C_COMPILER}" DIRECTORY)
    
    set(GCC_RUNTIME_DLL_NAMES 
        libstdc++-6.dll 
        libgcc_s_seh-1.dll
        libwinpthread-1.dll
    )
    
    set(GCC_RUNTIME_DLLS "")
    foreach(DLL_NAME IN LISTS GCC_RUNTIME_DLL_NAMES)
        find_file(CURRENT_DLL ${DLL_NAME} HINTS "${MINGW_BIN_DIR}")
        if(CURRENT_DLL)
            list(APPEND GCC_RUNTIME_DLLS "${CURRENT_DLL}")
        endif()
    endforeach()

    set(GLFW_DLL_NAME "glfw3.dll")
    get_filename_component(GLFW_LIB_DIR "${GLFW_LIBRARY_DIRS}" DIRECTORY)
    find_file(GLFW_DLL ${GLFW_DLL_NAME} HINTS "${GLFW_LIB_DIR}")
    
    if(NOT GLFW_DLL)
        message(FATAL_ERROR "Could not find ${GLFW_DLL_NAME} DLL. Build will fail.")
    endif()

    set(ALL_DLLS ${GCC_RUNTIME_DLLS} ${GLFW_DLL})

    add_custom_command(
        TARGET FireworkDance
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ALL_DLLS}
            $<TARGET_FILE_DIR:FireworkDance>
        COMMENT "Copying all required DLLs to the build directory..."
    )

else()
    message(STATUS "Skipping MinGW on Windows DLL copy.")
endif()
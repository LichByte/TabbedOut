# ============================================================================
# CMakeLists.txt - Build configuration for AltTabFix NVSE plugin
#
# HOW TO BUILD:
#   1. Open a terminal in the project folder
#   2. mkdir build && cd build
#   3. cmake .. -A Win32
#        (MUST be Win32/x86 -- FNV is a 32-bit game!)
#   4. cmake --build . --config Release
#   5. The DLL will be in build/Release/AltTabFix.dll
#
# Or open this folder in Visual Studio (File > Open > CMake)
# ============================================================================

cmake_minimum_required(VERSION 3.15)
project(AltTabFix VERSION 1.0.0 LANGUAGES CXX)

# FNV is 32-bit, so we MUST compile as x86
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(WARNING
        "Building for 64-bit! FalloutNV is 32-bit -- "
        "use 'cmake .. -A Win32' to target x86.")
endif()

# C++17 for modern features
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================================
# The plugin DLL
# ============================================================================
add_library(AltTabFix SHARED
    src/main.cpp
    src/d3d9_hook.cpp
    src/window_hook.cpp

    # Headers (listed so they show up in Visual Studio)
    src/d3d9_hook.h
    src/window_hook.h
    src/config.h
    src/utils.h
    nvse/PluginAPI.h
)

# Include directories
target_include_directories(AltTabFix PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link against DirectX 9 and Windows libraries
target_link_libraries(AltTabFix PRIVATE
    d3d9
    user32
    kernel32
    Ole32       # COM functions for WASAPI audio muting
)

# No "lib" prefix (we want "AltTabFix.dll", not "libAltTabFix.dll")
set_target_properties(AltTabFix PROPERTIES PREFIX "")

# Export the NVSE entry points
# (They're already __declspec(dllexport) in code, but just in case)
set_target_properties(AltTabFix PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS OFF
)

# ============================================================================
# Post-build: copy DLL to FNV's NVSE plugins folder if it exists
# Uncomment and edit the path below to auto-deploy when you build:
# ============================================================================
# set(FNV_PLUGINS_DIR "C:/Program Files (x86)/Steam/steamapps/common/Fallout New Vegas/Data/NVSE/Plugins")
# add_custom_command(TARGET AltTabFix POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy_if_different
#         $<TARGET_FILE:AltTabFix>
#         "${FNV_PLUGINS_DIR}/AltTabFix.dll"
#     COMMENT "Deploying AltTabFix.dll to FNV plugins folder"
# )

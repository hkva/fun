cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(Fun)

#
# Generator settings
#

# Build to bin/
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin)

# Export compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

#
# Shared options
#

add_library(common INTERFACE)

if(WIN32)
    target_compile_definitions(common INTERFACE _CRT_SECURE_NO_WARNINGS)
endif()

if(MSVC)
    target_compile_options(common INTERFACE /W4)
else()
    target_compile_options(common INTERFACE
        -g
        -Wall -Wextra
        -Wno-unused-parameter
    )
endif()

target_compile_features(common INTERFACE c_std_99)
target_compile_features(common INTERFACE cxx_std_17)

#
# Thirdparty code
#

include("${CMAKE_CURRENT_LIST_DIR}/thirdparty/CMakeLists.txt")

#
# Programs
#

# Math demo
add_executable(math "${CMAKE_CURRENT_LIST_DIR}/math.cc")
target_link_libraries(math PRIVATE common handmade-math)

# MD5 hash demo
add_executable(md5 "${CMAKE_CURRENT_LIST_DIR}/md5.cc")
target_link_libraries(md5 PRIVATE common)

# OpenGL demos
if(HAS_SDL AND HAS_OPENGL)
    add_library(opengl INTERFACE)
    target_link_libraries(opengl INTERFACE glad43 sdl stb imgui-sdl2 imgui-gl imgui)

    add_executable(opengl-cube "${CMAKE_CURRENT_LIST_DIR}/opengl-cube.cc")
    target_link_libraries(opengl-cube PRIVATE common opengl)
    add_executable(opengl-text "${CMAKE_CURRENT_LIST_DIR}/opengl-text.cc")
    target_link_libraries(opengl-text PRIVATE common opengl)
    add_executable(opengl-vector "${CMAKE_CURRENT_LIST_DIR}/opengl-vector.cc")
    target_link_libraries(opengl-vector PRIVATE common opengl nanosvg)
endif()

# Rectangle packing demo
if(HAS_SDL)
    add_executable(rect-packing "${CMAKE_CURRENT_LIST_DIR}/rect-packing.cc")
    target_link_libraries(rect-packing PRIVATE common sdl imgui-sdl2 imgui)
endif()

# WinAPI demos
if(WIN32)
    add_executable(winapi-tray WIN32
        "${CMAKE_CURRENT_LIST_DIR}/winapi-tray.c"
        "${CMAKE_CURRENT_LIST_DIR}/winapi-tray.rc"
    )
    target_link_libraries(winapi-tray PRIVATE common)
endif()